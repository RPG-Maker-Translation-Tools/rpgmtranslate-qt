#include "TaskPanel.hpp"
#include "MarkingTranslator.hpp"

#include <QLabel>
#include <QProgressBar>
#include <QPushButton>
#include <QSignalSpy>
#include <QtTest>

namespace {
auto makeTask(const u32 id, const TaskKind kind) -> TaskToken {
    return make_shared<Task>(id, kind);
}

auto labelsOf(const TaskPanel& panel) -> QStringList {
    QStringList texts;

    for (const QLabel* const label : panel.findChildren<QLabel*>()) {
        texts.append(label->text());
    }

    return texts;
}

auto placeholderOf(const TaskPanel& panel) -> QLabel* {
    return panel.findChildren<QLabel*>().at(0);
}
}  // namespace

class TestTaskPanel final : public QObject {
    Q_OBJECT

   private slots:
    void startsWithAPlaceholder() {
        const TaskPanel panel;

        QCOMPARE(panel.taskCount(), usize{ 0 });
        QCOMPARE(placeholderOf(panel)->text(), u"No tasks are running."_s);
        QVERIFY(!placeholderOf(panel)->isHidden());
    }

    void aTaskShowsItsNameAndAnUnknownProgress() {
        TaskPanel panel;
        QSignalSpy counts(&panel, &TaskPanel::taskCountChanged);

        panel.addTask(makeTask(1, TaskKind::Search));

        QCOMPARE(panel.taskCount(), usize{ 1 });
        QCOMPARE(counts.size(), qsizetype{ 1 });
        QCOMPARE(counts.at(0).at(0).toInt(), 1);
        QVERIFY(placeholderOf(panel)->isHidden());
        QVERIFY(labelsOf(panel).contains(u"Search"_s));

        // A range of 0 to 0 is the busy indicator, as the amount of work is not known yet
        const QProgressBar* const bar = panel.findChild<QProgressBar*>();
        QVERIFY(bar != nullptr);
        QCOMPARE(bar->minimum(), 0);
        QCOMPARE(bar->maximum(), 0);
    }

    void updatingATaskShowsTheFileAndTheProgress() {
        TaskPanel panel;
        panel.addTask(makeTask(7, TaskKind::Replace));

        panel.updateTask(7, u"actors.txt"_s, 3, 10);

        QVERIFY(labelsOf(panel).contains(u"Replace: actors.txt"_s));

        const QProgressBar* const bar = panel.findChild<QProgressBar*>();
        QCOMPARE(bar->maximum(), 10);
        QCOMPARE(bar->value(), 3);

        // An empty file name goes back to the bare name
        panel.updateTask(7, QString(), 10, 10);
        QVERIFY(labelsOf(panel).contains(u"Replace"_s));
    }

    void updatingAnUnknownTaskChangesNothing() {
        TaskPanel panel;
        panel.addTask(makeTask(1, TaskKind::Search));
        const QStringList before = labelsOf(panel);

        panel.updateTask(99, u"x.txt"_s, 1, 2);

        QCOMPARE(labelsOf(panel), before);
        QCOMPARE(panel.findChild<QProgressBar*>()->maximum(), 0);
    }

    void tasksAreTrackedByTheirId() {
        TaskPanel panel;
        panel.addTask(makeTask(1, TaskKind::Search));
        panel.addTask(makeTask(2, TaskKind::Statistics));
        QSignalSpy counts(&panel, &TaskPanel::taskCountChanged);

        panel.updateTask(2, u"b.txt"_s, 1, 4);

        QVERIFY(labelsOf(panel).contains(u"Statistics: b.txt"_s));
        QVERIFY(labelsOf(panel).contains(u"Search"_s));

        panel.removeTask(1);

        QCOMPARE(panel.taskCount(), usize{ 1 });
        QCOMPARE(counts.size(), qsizetype{ 1 });
        QCOMPARE(counts.at(0).at(0).toInt(), 1);
        QVERIFY(!labelsOf(panel).contains(u"Search"_s));
        QVERIFY(placeholderOf(panel)->isHidden());
    }

    void removingTheLastTaskBringsTheWordsBack() {
        TaskPanel panel;
        panel.show();
        panel.addTask(makeTask(1, TaskKind::Search));
        QSignalSpy counts(&panel, &TaskPanel::taskCountChanged);

        panel.removeTask(1);

        QCOMPARE(panel.taskCount(), usize{ 0 });
        QVERIFY(!placeholderOf(panel)->isHidden());
        QVERIFY(panel.isHidden());
        QCOMPARE(counts.at(0).at(0).toInt(), 0);
    }

    void removingAnUnknownTaskIsIgnored() {
        TaskPanel panel;
        panel.addTask(makeTask(1, TaskKind::Search));
        QSignalSpy counts(&panel, &TaskPanel::taskCountChanged);

        panel.removeTask(5);

        QCOMPARE(panel.taskCount(), usize{ 1 });
        QCOMPARE(counts.size(), qsizetype{ 0 });
    }

    void anInterruptibleTaskCanBeAborted() {
        TaskPanel panel;
        const TaskToken task = makeTask(1, TaskKind::Search);
        panel.addTask(task);

        auto* const button = panel.findChild<QPushButton*>();
        QVERIFY(button != nullptr);
        QVERIFY(!task->interrupted());

        button->click();

        QVERIFY(task->interrupted());
        QVERIFY(!button->isEnabled());
    }

    // A write or a read can't be stopped half way without leaving the game files torn
    void aTaskThatCannotBeInterruptedHasNoAbortButton() {
        TaskPanel panel;
        panel.addTask(makeTask(1, TaskKind::Write));

        QVERIFY(panel.findChild<QPushButton*>() == nullptr);
    }

    void changingTheLanguageRenamesTheTasksAndKeepsTheirFiles() {
        TaskPanel panel;
        panel.addTask(makeTask(1, TaskKind::Search));
        panel.updateTask(1, u"actors.txt"_s, 1, 2);

        const MarkingTranslator translator;
        translator.retranslate(&panel);

        QVERIFY(labelsOf(panel).contains(MarkingTranslator::MARK + u"Search: actors.txt"_s));
        QCOMPARE(placeholderOf(panel)->text(), MarkingTranslator::MARK + u"No tasks are running."_s);
    }
};

QTEST_MAIN(TestTaskPanel)
#include "tst_taskpanel.moc"
