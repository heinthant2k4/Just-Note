#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QFileDialog>
#include <QFile>
#include <QTextStream>
#include <QMessageBox>
#include <QInputDialog>
#include <QColorDialog>
#include <QTextCursor>
#include <QTextCharFormat>
#include <QFontDialog>
#include <QTimer>
#include <QTextBlock>
#include <QTextDocument>
#include <QLabel>
#include <QRegularExpression>
#include <QSettings>
#include <QStandardPaths>
#include <QTextStream>

// Constructor
MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent), ui(new Ui::MainWindow)
{
    ui->setupUi(this); // Setup the UI components
    autoSaveEnabled = false;  // Auto-save is initially disabled
    autoSaveTimer = new QTimer(this);  // Create the auto-save timer

    connect(autoSaveTimer, &QTimer::timeout, this, &MainWindow::autoSaveDocument); // Connect the timer signal to the auto-save function
    tabWidth = 4;
    useSpacesForTabs = true;

    // Initialize word count label
    wordCountLabel = new QLabel("Words: 0", this);
    statusBar()->addPermanentWidget(wordCountLabel);

    // Get document path
    QString documentsPath = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    QString settingsPath = documentsPath + "/NotepadAppSettings.ini";

    // Load search history from document
    QSettings settings(settingsPath, QSettings::IniFormat);
    searchHistory = settings.value("searchHistory").toStringList();
    // Initialize the tab widget and set it as the central widget
    tabWidget = new QTabWidget(this);
    tabWidget->setTabsClosable(true);
    tabWidget->setMovable(true);  // Enable drag-and-drop rearrangement
    setCentralWidget(tabWidget);

    // Connect the tab close signal
    connect(tabWidget, &QTabWidget::tabCloseRequested, this, &MainWindow::on_tabCloseRequested);

    // Open a new tab by default
    on_actionNew_triggered();
}

// Destructor
MainWindow::~MainWindow()
{
    delete ui; // Cleanup the UI components
}
//currentEdit mapping
QTextEdit* MainWindow::currentEditor()
{
    return qobject_cast<QTextEdit*>(tabWidget->currentWidget());
}

//tab
void MainWindow::on_tabCloseRequested(int index)
{
    QWidget *widget = tabWidget->widget(index);
    if (widget) {
        tabWidget->removeTab(index);
        delete widget;  // Delete the widget to free memory
    }
}

//saving
void MainWindow::closeEvent(QCloseEvent *event)
{
    // Get the path to the user's Documents directory
    QString documentsPath = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    QString settingsPath = documentsPath + "/Just-NoteSettings.ini";

    // Save search history to the settings file in the Documents directory
    QSettings settings(settingsPath, QSettings::IniFormat);
    settings.setValue("searchHistory", searchHistory);

    QMainWindow::closeEvent(event); // Call the base class implementation
}

// New file action: Clears current content
void MainWindow::on_actionNew_triggered()
{
    qDebug() << "New tab triggered";  // Debugging statement

    // Create a new text editor and add it to a new tab
    QTextEdit *editor = new QTextEdit(this);
    int tabIndex = tabWidget->addTab(editor, tr("Untitled"));
    tabWidget->setCurrentIndex(tabIndex);
}


// Open file action: Opens and reads a file into the text editor
void MainWindow::on_actionOpen_triggered()
{
    QString fileName = QFileDialog::getOpenFileName(this, tr("Open File"), "", tr("Text Files (*.txt);;All Files (*)"));
    if (!fileName.isEmpty()) {
        QFile file(fileName);
        if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QTextEdit *editor = new QTextEdit(this);
            editor->setPlainText(file.readAll());
            int tabIndex = tabWidget->addTab(editor, QFileInfo(fileName).fileName());
            tabWidget->setCurrentIndex(tabIndex);
            file.close();
        }
    }
}
// Save file action: Saves the current content to the current file
void MainWindow::on_actionSave_triggered()
{
    QTextEdit *editor = currentEditor();
    if (!editor) return;

           // Get the current tab's associated file path
    QString currentFile = tabFileMap.value(editor);

    if (currentFile.isEmpty()) {
        on_actionSave_As_triggered(); // If no file is currently associated, perform Save As
    } else {
        QFile file(currentFile);
        if (!file.open(QIODevice::WriteOnly | QFile::Text)) {
            QMessageBox::warning(this, "Warning", "Cannot save file: " + file.errorString());
            return;
        }
        QTextStream out(&file);
        QString text = editor->toPlainText(); // Get the plain text content from the current editor
        out << text; // Write content to file
        file.close(); // Close the file
        tabWidget->setTabText(tabWidget->currentIndex(), QFileInfo(currentFile).fileName()); // Update tab text
    }
}

// Save As action: Opens dialog to save current content to a new file
void MainWindow::on_actionSave_As_triggered()
{
    QTextEdit *editor = currentEditor();
    if (!editor) return;

    QString fileName = QFileDialog::getSaveFileName(this, tr("Save File"), "", tr("Text Files (*.txt);;All Files (*)"));
    if (!fileName.isEmpty()) {
        QFile file(fileName);
        if (!file.open(QIODevice::WriteOnly | QFile::Text)) {
            QMessageBox::warning(this, "Warning", "Cannot save file: " + file.errorString());
            return;
        }
        QTextStream out(&file);
        QString text = editor->toPlainText(); // Get the plain text content from the current editor
        out << text; // Write content to file
        file.close(); // Close the file

               // Update the file association and tab text
        tabFileMap[editor] = fileName;
        tabWidget->setTabText(tabWidget->currentIndex(), QFileInfo(fileName).fileName());
    }
}

// Undo action: Undo the last action in the text editor
void MainWindow::on_actionUndo_triggered()
{
    ui->textEdit->undo();
}

// Redo action: Redo the previously undone action in the text editor
void MainWindow::on_actionRedo_triggered()
{
    ui->textEdit->redo();
}

// Cut action: Cut the selected text
void MainWindow::on_actionCut_triggered()
{
    ui->textEdit->cut();
}

// Copy action: Copy the selected text
void MainWindow::on_actionCopy_triggered()
{
    ui->textEdit->copy();
}

// Paste action: Paste the copied text
void MainWindow::on_actionPaste_triggered()
{
    ui->textEdit->paste();
}

// Select All action: Selects all the text in the editor
void MainWindow::on_actionSelect_All_triggered()
{
    ui->textEdit->selectAll();
}

// Find action: Finds the text in the document
void MainWindow::on_actionFind_triggered()
{
    static QString lastSearchText = ""; // Keep track of the last searched text
    static QTextDocument::FindFlags lastSearchOptions; // Keep track of the last search options
    static QStringList searchHistory; // Search history list

    bool ok;

    // Prompt the user to select or enter the search text
    QString text = QInputDialog::getItem(this, tr("Find"), tr("Find what:"), searchHistory, 0, true, &ok);

        QTextDocument::FindFlags options;

               // Create a custom dialog for case sensitivity
        QMessageBox::StandardButton caseReply = QMessageBox::question(this, tr("Find"), tr("Match case?"), QMessageBox::Yes|QMessageBox::No);
        if (caseReply == QMessageBox::Yes) {
            options |= QTextDocument::FindCaseSensitively;
        }

        lastSearchText = text;
        lastSearchOptions = options;

        ui->textEdit->moveCursor(QTextCursor::Start); // Start from the beginning of the document

               // Perform the search
        if (!ui->textEdit->find(text, options)) {
            QMessageBox::information(this, tr("Find"), tr("Cannot find \"%1\"").arg(text)); // Show message if text not found
            return;
        }

               // Loop to find next/previous occurrences
        while (true) {
            QMessageBox::StandardButton nextReply = QMessageBox::question(this, tr("Find Next"), tr("Do you want to find the next occurrence?"), QMessageBox::Yes|QMessageBox::No|QMessageBox::Cancel);

            if (nextReply == QMessageBox::Yes) {
                if (!ui->textEdit->find(text, options)) {
                    QMessageBox::information(this, tr("Find Next"), tr("No more occurrences of \"%1\"").arg(text));
                    break;
                }
            } else if (nextReply == QMessageBox::No) {
                nextReply = QMessageBox::question(this, tr("Find Previous"), tr("Do you want to find the previous occurrence?"), QMessageBox::Yes|QMessageBox::Cancel);

                if (nextReply == QMessageBox::Yes) {
                    if (!ui->textEdit->find(text, options | QTextDocument::FindBackward)) {
                        QMessageBox::information(this, tr("Find Previous"), tr("No previous occurrences of \"%1\"").arg(text));
                        break;
                    }
                } else {
                    break;
                }
            } else {
                break;
            }
        }
    }


// Replace action: Replaces occurrences of text in the document
void MainWindow::on_actionReplace_triggered()
{
    // Prompt the user for the text to find
    bool ok;
    QString findText = QInputDialog::getText(this, tr("Find"), tr("Find what:"), QLineEdit::Normal, "", &ok);

    if (ok && !findText.isEmpty()) {
        // Prompt the user for the replacement text
        QString replaceText = QInputDialog::getText(this, tr("Replace"), tr("Replace with:"), QLineEdit::Normal, "", &ok);
        if (ok) {
            QTextCursor cursor = ui->textEdit->textCursor(); // Get the current cursor
            QTextDocument *document = ui->textEdit->document(); // Get the document

            cursor.beginEditBlock(); // Begin the undo block

            int count = 0; // Counter for replacements

                   // Move the cursor to the start of the document
            cursor.movePosition(QTextCursor::Start);

                   // Loop through the document to find all occurrences
            while (!cursor.isNull() && !cursor.atEnd()) {
                // Find the next occurrence of the text
                cursor = document->find(findText, cursor);

                       // Check if text is found and selected
                if (!cursor.isNull() && cursor.hasSelection()) {
                    QTextCharFormat originalFormat = cursor.charFormat(); // Save the original formatting

                    cursor.insertText(replaceText); // Replace the found text

                    cursor.setCharFormat(originalFormat); // Restore the original formatting
                    count++; // Increment the counter
                }
            }

            cursor.endEditBlock(); // End the undo block

                   // Inform the user about the number of replacements made
            QMessageBox::information(this, tr("Replace"), tr("Replaced %1 occurrences of '%2'.").arg(count).arg(findText));
        }
    }
}



// Highlight action: Opens a color picker dialog to highlight text
void MainWindow::on_actionHighlight_triggered()
{
    QColor color = QColorDialog::getColor(Qt::yellow, this, "Select Highlight Color"); // Prompt for highlight color

    if (color.isValid()) {
        QTextCursor cursor = ui->textEdit->textCursor();

        if (!cursor.selectedText().isEmpty()) {
            QTextCharFormat format;
            format.setBackground(color); // Set background color for highlight

            cursor.mergeCharFormat(format); // Apply formatting to selected text
        }
    }
}

// Highlight predefined colors: Yellow
void MainWindow::on_actionHighlight_Yellow_triggered()
{
    highlightTextWithColor(Qt::yellow);
}

// Highlight predefined colors: Green
void MainWindow::on_actionHighlight_Green_triggered()
{
    highlightTextWithColor(Qt::green);
}

// Highlight predefined colors: Blue
void MainWindow::on_actionHighlight_Blue_triggered()
{
    highlightTextWithColor(Qt::blue);
}

// Helper function: Apply the selected color to the text
void MainWindow::highlightTextWithColor(const QColor &color)
{
    QTextCursor cursor = ui->textEdit->textCursor();

    if (!cursor.selectedText().isEmpty()) {
        QTextCharFormat format;
        format.setBackground(color); // Set background color
        cursor.mergeCharFormat(format); // Apply formatting
    }
}

// Clear highlight action: Clears the highlight color
void MainWindow::on_actionClear_Highlight_triggered()
{
    highlightTextWithColor(Qt::transparent); // Clear highlight by setting color to transparent
}

// Bold action: Toggles bold formatting on the selected text
void MainWindow::on_actionBold_triggered() {
    QTextCursor cursor = ui->textEdit->textCursor();

    if (!cursor.hasSelection()) {
        // Apply format to new text or the cursor position
        QTextCharFormat format;
        format.setFontWeight(cursor.charFormat().fontWeight() == QFont::Bold ? QFont::Normal : QFont::Bold);
        ui->textEdit->setCurrentCharFormat(format);
        return;
    }

    QTextCharFormat format = cursor.charFormat();
    format.setFontWeight(format.fontWeight() == QFont::Bold ? QFont::Normal : QFont::Bold);
    cursor.mergeCharFormat(format);
}

// Italic action: Toggles italic formatting on the selected text
void MainWindow::on_actionItalic_triggered() {
    QTextCursor cursor = ui->textEdit->textCursor();

    if (!cursor.hasSelection()) {
        QTextCharFormat format;
        format.setFontItalic(!cursor.charFormat().fontItalic());
        ui->textEdit->setCurrentCharFormat(format);
        return;
    }

    QTextCharFormat format = cursor.charFormat();
    format.setFontItalic(!format.fontItalic());
    cursor.mergeCharFormat(format);
}

// Underline action: Toggles underline formatting on the selected text
void MainWindow::on_actionUnderline_triggered() {
    QTextCursor cursor = ui->textEdit->textCursor();

    if (!cursor.hasSelection()) {
        QTextCharFormat format;
        format.setFontUnderline(!cursor.charFormat().fontUnderline());
        ui->textEdit->setCurrentCharFormat(format);
        return;
    }

    QTextCharFormat format = cursor.charFormat();
    format.setFontUnderline(!format.fontUnderline());
    cursor.mergeCharFormat(format);
}

// Strikethrough action: Toggles strikethrough formatting on the selected text
void MainWindow::on_actionStrike_through_triggered() {
    QTextCursor cursor = ui->textEdit->textCursor();

    if (!cursor.hasSelection()) {
        QTextCharFormat format;
        format.setFontStrikeOut(!cursor.charFormat().fontStrikeOut());
        ui->textEdit->setCurrentCharFormat(format);
        return;
    }

    QTextCharFormat format = cursor.charFormat();
    format.setFontStrikeOut(!format.fontStrikeOut());
    cursor.mergeCharFormat(format);
}

// Subscript action: Toggles subscript formatting on the selected text
void MainWindow::on_actionSub_Script_triggered() {
    QTextCursor cursor = ui->textEdit->textCursor();

    if (!cursor.hasSelection()) {
        QTextCharFormat format;
        format.setVerticalAlignment(cursor.charFormat().verticalAlignment() == QTextCharFormat::AlignSubScript
                                            ? QTextCharFormat::AlignNormal
                                            : QTextCharFormat::AlignSubScript);
        ui->textEdit->setCurrentCharFormat(format);
        return;
    }

    QTextCharFormat format = cursor.charFormat();
    format.setVerticalAlignment(format.verticalAlignment() == QTextCharFormat::AlignSubScript
                                        ? QTextCharFormat::AlignNormal
                                        : QTextCharFormat::AlignSubScript);
    cursor.mergeCharFormat(format);
}

// Superscript action: Toggles superscript formatting on the selected text
void MainWindow::on_actionSuper_Script_triggered() {
    QTextCursor cursor = ui->textEdit->textCursor();

    if (!cursor.hasSelection()) {
        QTextCharFormat format;
        format.setVerticalAlignment(cursor.charFormat().verticalAlignment() == QTextCharFormat::AlignSuperScript
                                            ? QTextCharFormat::AlignNormal
                                            : QTextCharFormat::AlignSuperScript);
        ui->textEdit->setCurrentCharFormat(format);
        return;
    }

    QTextCharFormat format = cursor.charFormat();
    format.setVerticalAlignment(format.verticalAlignment() == QTextCharFormat::AlignSuperScript
                                        ? QTextCharFormat::AlignNormal
                                        : QTextCharFormat::AlignSuperScript);
    cursor.mergeCharFormat(format);
}


//font change action
void MainWindow::on_actionFont_Style_triggered() {
    bool ok;
    QFont font = QFontDialog::getFont(&ok, this);
    if (ok) {
        QTextCursor cursor = ui->textEdit->textCursor();
        QTextCharFormat format;
        format.setFont(font);
        cursor.mergeCharFormat(format);
    }
}

//line spacing action
void MainWindow::on_actionSpacing_triggered(){
    // Prompt the user to enter the desired line spacing
    qDebug() << "Line Spacing triggered";
    bool ok;
    double spacing = QInputDialog::getDouble(this, "Line Spacing",
                                             "Enter line spacing (e.g., 1.0 for single, 2.0 for double):",
                                             1.0, 0.5, 4.0, 1, &ok);

    if (ok) { // If the user clicked OK and entered a valid number
        QTextCursor cursor = ui->textEdit->textCursor(); // Get the current text cursor
        QTextBlockFormat blockFormat = cursor.blockFormat(); // Get the format of the current text block

        // Set the line height to the specified value (proportional to single spacing)
        blockFormat.setLineHeight(spacing * 100, QTextBlockFormat::ProportionalHeight);

               // Apply the new format to the current text block
        cursor.setBlockFormat(blockFormat);
    }
}

//Clear format
void MainWindow::on_actionClear_All_Format_triggered()
{
    QTextCursor cursor = ui->textEdit->textCursor(); // Get the current text cursor

    if (!cursor.hasSelection()) {
        // If no text is selected, select the entire document
        cursor.select(QTextCursor::Document);
    }

           // Create a new format with default settings (no formatting)
    QTextCharFormat defaultFormat;
    defaultFormat.setFontWeight(QFont::Normal);
    defaultFormat.setFontItalic(false);
    defaultFormat.setFontUnderline(false);
    defaultFormat.setFontStrikeOut(false);
    defaultFormat.setVerticalAlignment(QTextCharFormat::AlignNormal);
    defaultFormat.setForeground(QBrush(Qt::black)); // Default text color
    defaultFormat.setBackground(QBrush(Qt::white)); // Default background color

    cursor.mergeCharFormat(defaultFormat); // Apply the default formatting
}

//Font color
void MainWindow::on_actionText_Color_triggered()
{
    // Open a color dialog to select a text color
    QColor color = QColorDialog::getColor(ui->textEdit->textColor(), this, tr("Select Text Color"));

    if (color.isValid()) {
        // Apply the selected text color to the text editor
        ui->textEdit->setTextColor(color);
    }
}

//background action
void MainWindow::on_actionBackground_Color_triggered()
{
    // Open a color dialog to select a background color
    QColor color = QColorDialog::getColor(ui->textEdit->palette().color(QPalette::Base), this, tr("Select Background Color"));

    if (color.isValid()) {
        // Set the background color of the text editor
        QPalette p = ui->textEdit->palette();
        p.setColor(QPalette::Base, color); // Set the base color (background)
        ui->textEdit->setPalette(p);
    }
}

//dark mode
void MainWindow::on_actionDark_Mode_triggered()
{
    // Check the current palette's base color to determine if dark mode is enabled
    QPalette p = ui->textEdit->palette();
    QColor currentColor = p.color(QPalette::Base);

    if (currentColor == Qt::white) {
        // Switch to dark mode
        p.setColor(QPalette::Base, QColor(53, 53, 53)); // Dark background color
        p.setColor(QPalette::Text, Qt::white); // White text color
    } else {
        // Switch to light mode
        p.setColor(QPalette::Base, Qt::white); // Light background color
        p.setColor(QPalette::Text, Qt::black); // Black text color
    }

    ui->textEdit->setPalette(p);
}


// Save and save interval
void MainWindow::on_actionAuto_Save_triggered()
{
    autoSaveEnabled = !autoSaveEnabled;  // Toggle the auto-save state

    if (autoSaveEnabled) {
        // Enable auto-save with the default interval (e.g., 5 minutes)
        autoSaveTimer->start(300000); // 5 minutes in milliseconds
        QMessageBox::information(this, tr("Auto-Save"), tr("Auto-save enabled. Documents will be saved every 5 minutes."));
    } else {
        // Disable auto-save
        autoSaveTimer->stop();
        QMessageBox::information(this, tr("Auto-Save"), tr("Auto-save disabled."));
    }
}

//save interval function
void MainWindow::on_actionSave_Interval_triggered()
{
    // Prompt user for auto-save interval in minutes
    bool ok;
    int interval = QInputDialog::getInt(this, tr("Auto-Save Interval"), tr("Set auto-save interval (in minutes):"), 5, 1, 60, 1, &ok);

    if (ok) {
        autoSaveTimer->setInterval(interval * 60000); // Convert minutes to milliseconds
        QMessageBox::information(this, tr("Auto-Save"), tr("Auto-save interval set to %1 minutes.").arg(interval));
    }
}


// Function to auto-save the document
void MainWindow::autoSaveDocument(){
    if (!currentFile.isEmpty()){
        on_actionSave_triggered(); // save the file
    }
}

//tab width and spacesfortabs
void MainWindow::on_actionTab_Width_triggered(){
    bool ok;
    int width = QInputDialog::getInt(this, tr("Tab Width"), tr("Set tab width (number of spaces):"), tabWidth, 1, 8, 1, &ok);

    if (ok) {
        tabWidth = width;
        ui->textEdit->setTabStopDistance(tabWidth * QFontMetricsF(ui->textEdit->font()).horizontalAdvance(' ')); // Set tab width
        QMessageBox::information(this, tr("Tab Width"), tr("Tab width set to %1 spaces.").arg(tabWidth));
    }
}

void MainWindow::on_actionToggle_Spaces_for_Tabs_triggered(){
    useSpacesForTabs = !useSpacesForTabs;
    QMessageBox::information(this, tr("Spaces for Tabs"), useSpacesForTabs ? tr("Using spaces for tabs.") : tr("Using tabs."));
}

// Update the word count display in the status bar
void MainWindow::updateWordCount()
{
    // Get the text from the editor
    QString text = ui->textEdit->toPlainText();

           // Calculate the word count
    int wordCount = text.split(QRegularExpression("(\\s|\\n|\\r)+"), Qt::SkipEmptyParts).count();

           // Update the word count label
    wordCountLabel->setText(QString("Words: %1").arg(wordCount));
}

//alignment
void MainWindow::on_actionLeft_triggered() {
    ui->textEdit->setAlignment(Qt::AlignLeft);
}

void MainWindow::on_actionCenter_triggered() {
    ui->textEdit->setAlignment(Qt::AlignCenter);
}

void MainWindow::on_actionRight_triggered() {
    ui->textEdit->setAlignment(Qt::AlignRight);
}

void MainWindow::on_actionJustify_triggered() {
    ui->textEdit->setAlignment(Qt::AlignJustify);
}


// Exit action: Closes the application
void MainWindow::on_actionExit_triggered()
{
    QApplication::quit();
}
