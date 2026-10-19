#pragma once

#include "Aliases.hpp"

#include <QString>
#include <QWidget>

// A Yes/No question like QMessageBox::question, whose `details` sit in a scrollable box instead of growing the dialog
// past the screen. Returns whether Yes was chosen.
[[nodiscard]] auto askWithDetails(QWidget* parent, const QString& title, const QString& text, const QString& details)
    -> bool;
