/*
    SPDX-FileCopyrightText: 2014 Martin Gräßlin <mgraesslin@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "clipboardjob.h"
#include "history.h"
#include "historyitem.h"
#include "historystringitem.h"
#include "klipper.h"

#include "klipper_debug.h"
#include <KIO/PreviewJob>
#include <QFutureWatcher>
#include <QIcon>
#include <QtConcurrent>

#include <Prison/Prison>

const static QString s_iconKey = QStringLiteral("icon");
const static QString s_previewKey = QStringLiteral("preview");
const static QString s_previewWidthKey = QStringLiteral("previewWidth");
const static QString s_previewHeightKey = QStringLiteral("previewHeight");
const static QString s_urlKey = QStringLiteral("url");

ClipboardJob::ClipboardJob(Klipper *klipper, const QString &destination, const QString &operation, const QVariantMap &parameters, QObject *parent)
    : Plasma5Support::ServiceJob(destination, operation, parameters, parent)
    , m_klipper(klipper)
{
}

void ClipboardJob::start()
{
    const QString operation = operationName();
    // first check for operations not needing an item
    if (operation == QLatin1String("clearHistory")) {
        m_klipper->slotAskClearHistory();
        setResult(true);
        return;
    } else if (operation == QLatin1String("configureKlipper")) {
        m_klipper->slotConfigure();
        setResult(true);
        return;
    }

    // other operations need the item
    HistoryItemConstPtr item = m_klipper->history()->find(QByteArray::fromBase64(destination().toUtf8()));
    if (!item) {
        setResult(false);
        return;
    }
    if (operation == QLatin1String("select")) {
        m_klipper->history()->slotMoveToTop(item->uuid());
        setResult(true);
    } else if (operation == QLatin1String("remove")) {
        m_klipper->history()->remove(item);
        setResult(true);
    } else if (operation == QLatin1String("edit")) {
        if (parameters().contains(QLatin1String("text"))) {
            const QString text = parameters()[QLatin1String("text")].toString();
            m_klipper->history()->remove(item);
            m_klipper->history()->insert(HistoryItemPtr(new HistoryStringItem(text)));
            if (m_klipper->urlGrabber()) {
                m_klipper->urlGrabber()->checkNewData(HistoryItemConstPtr(m_klipper->history()->first()));
            }
            setResult(true);
            return;
        }
        connect(m_klipper, &Klipper::editFinished, this, [this, item](HistoryItemConstPtr editedItem, int result) {
            if (item != editedItem) {
                // not our item
                return;
            }
            setResult(result);
        });
        m_klipper->editData(item);
        return;
    } else if (operation == QLatin1String("barcode")) {
        int pixelWidth = parameters().value(QStringLiteral("width")).toInt();
        int pixelHeight = parameters().value(QStringLiteral("height")).toInt();
        Prison::AbstractBarcode *code = nullptr;
        switch (parameters().value(QStringLiteral("barcodeType")).toInt()) {
        case 1: {
            code = Prison::createBarcode(Prison::DataMatrix);
            const int size = qMin(pixelWidth, pixelHeight);
            pixelWidth = size;
            pixelHeight = size;
            break;
        }
        case 2: {
            code = Prison::createBarcode(Prison::Code39);
            break;
        }
        case 3: {
            code = Prison::createBarcode(Prison::Code93);
            break;
        }
        case 4: {
            code = Prison::createBarcode(Prison::Aztec);
            break;
        }
        case 0:
        default: {
            code = Prison::createBarcode(Prison::QRCode);
            const int size = qMin(pixelWidth, pixelHeight);
            pixelWidth = size;
            pixelHeight = size;
            break;
        }
        }
        if (code) {
            code->setData(item->text());
            QFutureWatcher<QImage> *watcher = new QFutureWatcher<QImage>(this);
            connect(watcher, &QFutureWatcher<QImage>::finished, this, [this, watcher, code] {
                setResult(watcher->result());
                watcher->deleteLater();
                delete code;
            });
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
            auto future = QtConcurrent::run(code, &Prison::AbstractBarcode::toImage, QSizeF(pixelWidth, pixelHeight));
#else
            auto future = QtConcurrent::run(&Prison::AbstractBarcode::toImage, code, QSizeF(pixelWidth, pixelHeight));
#endif
            watcher->setFuture(future);
            return;
        } else {
            setResult(false);
        }
    } else if (operation == QLatin1String("action")) {
        m_klipper->urlGrabber()->invokeAction(item);
        setResult(true);

    } else if (operation == s_previewKey) {
        const int pixelWidth = parameters().value(s_previewWidthKey).toInt();
        const int pixelHeight = parameters().value(s_previewHeightKey).toInt();
        QUrl url = parameters().value(s_urlKey).toUrl();
        qCDebug(KLIPPER_LOG) << "URL: " << url;
        KFileItem item(url);

        if (pixelWidth <= 0 || pixelHeight <= 0) {
            qCWarning(KLIPPER_LOG) << "Preview size invalid: " << pixelWidth << "x" << pixelHeight;
            iconResult(item);
            return;
        }

        if (!url.isValid() || !url.isLocalFile()) { // no remote files
            qCWarning(KLIPPER_LOG) << "Invalid or non-local url for preview: " << url;
            iconResult(item);
            return;
        }

        KFileItemList urls;
        urls << item;

        KIO::PreviewJob *job = KIO::filePreview(urls, QSize(pixelWidth, pixelHeight));
        job->setIgnoreMaximumSize(true);
        connect(job, &KIO::PreviewJob::gotPreview, this, [this](const KFileItem &item, const QPixmap &preview) {
            QVariantMap res;
            res.insert(s_urlKey, item.url());
            res.insert(s_previewKey, preview);
            res.insert(s_iconKey, false);
            res.insert(s_previewWidthKey, preview.size().width());
            res.insert(s_previewHeightKey, preview.size().height());
            setResult(res);
        });
        connect(job, &KIO::PreviewJob::failed, this, [this](const KFileItem &item) {
            iconResult(item);
        });

        job->start();

        return;
    } else {
        setResult(false);
    }
    emitResult();
}

void ClipboardJob::iconResult(const KFileItem &item)
{
    QVariantMap res;
    res.insert(s_urlKey, item.url());
    QPixmap pix = QIcon::fromTheme(item.determineMimeType().iconName()).pixmap(128, 128);
    res.insert(s_previewKey, pix);
    res.insert(s_iconKey, true);
    res.insert(QStringLiteral("iconName"), item.currentMimeType().iconName());
    res.insert(s_previewWidthKey, pix.size().width());
    res.insert(s_previewHeightKey, pix.size().height());
    setResult(res);
}
