#pragma once

#include "Aliases.hpp"
#include "FWD.hpp"

#include <QDialog>

class ScriptEditor final : public QDialog {
   public:
    ScriptEditor(QWidget* parent, const QString& name, const QString& source, bool readOnly);

    [[nodiscard]] auto source() const -> QString;

   private:
    CodeViewer* const editor;
};
