/*
    SPDX-FileCopyrightText: 2019 Nicolas Fella <nicolas.fella@gmx.de>
    SPDX-FileCopyrightText: 2020 Carson Black <uhhadd@gmail.com>

    SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
*/

import QtQuick 2.6
import QtQuick.Layouts 1.3
import QtQuick.Controls 2.5 as QQC2

import org.kde.kcm 1.2 as KCM
import org.kde.kirigami 2.13 as Kirigami
import org.kde.plasma.kcm.users 1.0 as UsersKCM

KCM.ScrollViewKCM {
    id: root

    title: i18n("Manage Users")
    // Make the first page a sidebar
    Kirigami.ColumnView.fillWidth: false
    LayoutMirroring.enabled: Qt.application.layoutDirection === Qt.RightToLeft
    LayoutMirroring.childrenInherit: true

    implicitWidth: Kirigami.Units.gridUnit * 30
    implicitHeight: Kirigami.Units.gridUnit * 20

    // Override default false value here as it doesn't really work with the
    // layout we have here with two pages adjacent to one another
    framedView: true

    // QML cannot update avatar image when override. By increasing this number and
    // appending it to image source with '?', we force avatar to reload
    property int avatarVersion: 0

    function createUser(userName, realName, password, isAdministrator) {
        if (kcm.createUser(userName, realName, password, isAdministrator)) {
            userList.indexToActivate = userList.count;
        }
    }
    function deleteUser(uid, deleteData) {
        if (kcm.deleteUser(uid, deleteData)) {
            userList.indexToActivate = userList.count - 1;
        }
    }

    Connections {
        target: kcm
        function onApply() {
            avatarVersion += 1
        }
    }

    Component.onCompleted: {
        kcm.columnWidth = Kirigami.Units.gridUnit * 15
        kcm.push("UserDetailsPage.qml", { user: kcm.userModel.getLoggedInUser() })
    }

    view: ListView {
        id: userList

        property int indexToActivate: -1

        model: kcm.userModel

        onCountChanged: {
            if (indexToActivate >= 0) {
                kcm.pop();
                currentIndex = Math.min(indexToActivate, count - 1);
                const modelIndex = kcm.userModel.index(currentIndex, 0);
                const user = kcm.userModel.data(modelIndex, UsersKCM.UserModel.UserRole);
                kcm.push("UserDetailsPage.qml", { user });
                indexToActivate = -1;
            }
        }
        section {
            property: "sectionHeader"
            delegate: Kirigami.ListSectionHeader {
                label: section
            }
        }

        delegate: Kirigami.BasicListItem {
            property UsersKCM.User user: model.userObject

            text: model.displayPrimaryName
            subtitle: model.displaySecondaryName
            reserveSpaceForSubtitle: true

            highlighted: index === userList.currentIndex

            onClicked: {
                userList.currentIndex = index;
                kcm.pop(0);
                kcm.push("UserDetailsPage.qml", { user });
            }

            Kirigami.Theme.colorSet: highlighted ? Kirigami.Theme.Selection : Kirigami.Theme.View

            leading: Rectangle {
                width: height

                color: "transparent"
                border.color: Kirigami.ColorUtils.adjustColor(Kirigami.Theme.textColor, { alpha: 0.4 * 255 })
                border.width: 1
                radius: height / 2

                Kirigami.Avatar {
                    source: model.decoration + '?' + avatarVersion // force reload after saving
                    cache: false // avoid caching
                    name: model.displayPrimaryName
                    anchors {
                        fill: parent
                        margins: 1
                    }
                }
            }
        }
    }

    footer: RowLayout {
        QQC2.Button {
            icon.name: "list-add"

            text: i18n("Add New User")

            onClicked: {
                kcm.pop(0);
                kcm.push("CreateUser.qml");
                userList.currentIndex = -1;
            }
        }

        Item {
            Layout.fillWidth: true
        }
    }

    Item {} // For some reason, this being here keeps the layout from breaking.
}
