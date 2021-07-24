/*
    SPDX-FileCopyrightText: 2021 Vlad Zahorodnii <vlad.zahorodnii@gmail.com>

    SPDX-License-Identifier: LGPL-2.1-or-later
*/

#include <QCoreApplication>
#include <QDebug>
#include <QString>

#include "differ.h"

int main(int argc, char *argv[])
{
    using namespace differ;
    QCoreApplication a(argc, argv);

    const QString src = argv[1];
    const QString dst = argv[2];

    const auto operations = diff(src, dst, DiffOption::DetectMoves);
    for (const auto &operation : operations) {
        if (auto insertOperation = std::get_if<InsertOperation>(&operation)) {
            qDebug() << "insert" << dst[insertOperation->offset] << "at" << insertOperation->index;
        } else if (auto removeOperation = std::get_if<RemoveOperation>(&operation)) {
            qDebug() << "remove" << removeOperation->count << "items at" << removeOperation->offset;
        } else if (auto moveOperation = std::get_if<MoveOperation>(&operation)) {
            qDebug() << "move from" << moveOperation->from << "to" << moveOperation->to;
        }
    }

    return 0;
}
