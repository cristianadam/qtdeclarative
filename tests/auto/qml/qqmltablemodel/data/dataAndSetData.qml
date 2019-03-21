/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

import QtQuick 2.12
import Qt.labs.qmlmodels 1.0

TableView {
    width: 200; height: 200
    model: TableModel {
        id: testModel
        objectName: "testModel"
        rows: [
            [
                { name: "John", someOtherRole1: "foo" },    // column 0
                { age: 22, someOtherRole2: "foo" }          // column 1
            ],
            [
                { name: "Oliver", someOtherRole1: "foo" },  // column 0
                { age: 33, someOtherRole2: "foo" }          // column 1
            ]
        ]

        // This is silly: in real life, store the birthdate instead of the age,
        // and let the delegate calculate the age, so it won't need updating
        function happyBirthday(dude) {
            var row = -1;
            for (var r = 0; row < 0 && r < testModel.rowCount; ++r)
                if (testModel.data(testModel.index(r, 0), "name") === dude)
                    row = r;
            var index = testModel.index(row, 1)
            testModel.setData(index, "age", testModel.data(index, "age") + 1)
        }
    }
    delegate: Text {
        id: textItem
        text: model.display
        TapHandler {
            onTapped: testModel.happyBirthday(testModel.data(testModel.index(row, 0), "name"))
        }
    }
}