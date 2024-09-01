#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTextEdit>
#include <QLabel>
#include <QCloseEvent>
#include <QTabWidget>
#include <QTextStream>
#include <QCompleter>
#include <QStringListModel>
#include <QFile>
#include <QtTextToSpeech/QTextToSpeech>

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

public slots:
    void on_actionNew_triggered();
    void on_actionOpen_triggered();
    void on_actionSave_triggered();
    void on_actionSave_As_triggered();
    void on_actionExit_triggered();
    void on_actionUndo_triggered();
    void on_actionRedo_triggered();
    void on_actionCut_triggered();
    void on_actionCopy_triggered();
    void on_actionPaste_triggered();
    void on_actionSelect_All_triggered();
    void on_actionFind_triggered();
    void on_actionReplace_triggered();
    void on_actionHighlight_triggered();
    void on_actionHighlight_Yellow_triggered();
    void on_actionHighlight_Green_triggered();
    void on_actionHighlight_Blue_triggered();  // Corrected this line
    void on_actionClear_Highlight_triggered();
    void on_actionBold_triggered();
    void on_actionItalic_triggered();
    void on_actionUnderline_triggered();
    void on_actionStrike_through_triggered();
    void on_actionSub_Script_triggered();
    void on_actionSuper_Script_triggered();
    void on_actionFont_Style_triggered();
    void on_actionSpacing_triggered();
    void on_actionClear_All_Format_triggered();
    void on_actionText_Color_triggered();
    void on_actionBackground_Color_triggered();
    void on_actionDark_Mode_triggered();
    void on_actionAuto_Save_triggered();
    void on_actionSave_Interval_triggered();
    void autoSaveDocument();
    void on_actionTab_Width_triggered();
    void on_actionLeft_triggered();
    void on_actionRight_triggered();
    void on_actionCenter_triggered();
    void on_actionJustify_triggered();

private slots:
    void highlightTextWithColor(const QColor &color);
    void updateWordCount();
    void on_tabCloseRequested(int index);
    void on_actionAdd_Bullet_Points_triggered();
    void on_actionAdd_Numberings_triggered();
    void on_actionText_To_Speech_triggered();

private:
    Ui::MainWindow *ui;
    QString currentFile;
    QTimer *autoSaveTimer;  // Timer for auto-save functionality
    bool autoSaveEnabled;    // Flag to keep track of auto-save state
    int tabWidth;
    bool useSpacesForTabs;
    QLabel *wordCountLabel;
    QStringList searchHistory;
    void closeEvent(QCloseEvent *event) override;
    QTabWidget *tabWidget;
    QTextEdit *currentEditor();
    QMap<QWidget*, QString> tabFileMap; // Map each tab's widget to its associated file path
    bool isDarkmode;
    QTextToSpeech *speech;


};

#endif // MAINWINDOW_H
