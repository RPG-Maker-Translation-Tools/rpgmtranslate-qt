#include "TranslationTableModel.hpp"

#include "Constants.hpp"

TranslationTableModel::TranslationTableModel(QObject* const parent) :
    QStandardItemModel(parent) {}

auto TranslationTableModel::setData(
    const QModelIndex& index,
    const QVariant& value,
    const i32 role
) -> bool {
    if (!index.isValid()) {
        return QStandardItemModel::setData(index, value, role);
    }

    if (role == Qt::EditRole || role == Qt::DisplayRole) {
        if (item(index.row(), 0)->text() == BOOKMARK_COMMENT) {
            const bool result = QStandardItemModel::setData(index, value, role);
            emit bookmarkChanged(index.row());
            return result;
        }

        const QString oldText = data(index, Qt::EditRole).toString();
        const QString newText = value.toString();

        const bool oldTranslated = !QStringView(oldText).trimmed().isEmpty();
        const bool newTranslated = !QStringView(newText).trimmed().isEmpty();

        if (oldTranslated != newTranslated) {
            emit translatedChanged(newTranslated ? +1 : -1);
        }
    }

    return QStandardItemModel::setData(index, value, role);
}

[[nodiscard]] auto TranslationTableModel::flags(const QModelIndex& index) const
    -> Qt::ItemFlags {
    Qt::ItemFlags flags = QStandardItemModel::flags(index);

    const QModelIndex controlIndex = this->index(index.row(), 1);
    QStandardItem* const controlItem = itemFromIndex(controlIndex);

    if ((controlItem != nullptr) && !controlItem->isEditable()) {
        flags.setFlag(Qt::ItemIsEditable, false);
    }

    return flags;
}
