/*
    SPDX-FileCopyrightText: 2019 Kai Uwe Broulik <kde@privat.broulik.de>

    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

import QtQuick 2.8

import org.kde.plasma.core 2.0 as PlasmaCore
import org.kde.plasma.extras 2.0 as PlasmaExtras

import org.kde.kquickcontrolsaddons 2.0 as KQCAddons

PlasmaExtras.Menu {
    id: contextMenu

    signal closed

    property QtObject __clipboard: KQCAddons.Clipboard { }

    // can be a Text or TextEdit
    property Item target

    property string link

    onStatusChanged: {
        if (status === PlasmaExtras.DialogStatus.Closed) {
            closed();
        }
    }

    PlasmaExtras.MenuItem {
        text: i18nd("plasma_applet_org.kde.plasma.notifications", "Copy Link Address")
        onClicked: __clipboard.content = contextMenu.link
        visible: contextMenu.link !== ""
    }

    PlasmaExtras.MenuItem {
        separator: true
        visible: contextMenu.link !== ""
    }

    PlasmaExtras.MenuItem {
        text: i18nd("plasma_applet_org.kde.plasma.notifications", "Copy")
        icon: "edit-copy"
        enabled: typeof target.selectionStart !== "undefined"
        ? target.selectionStart !== target.selectionEnd
        : (target.text || "").length > 0
        onClicked: {
            if (typeof target.copy === "function") {
                target.copy();
            } else {
                __clipboard.content = target.text;
            }
        }
    }

    PlasmaExtras.MenuItem {
        id: selectAllAction
        icon: "edit-select-all"
        text: i18nd("plasma_applet_org.kde.plasma.notifications", "Select All")
        onClicked: target.selectAll()
        visible: typeof target.selectAll === "function"
    }
}
