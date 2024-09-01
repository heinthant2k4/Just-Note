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
#include <QTextTable>
#include <QTextList>
#include <QTextBlockFormat>

// Constructor
MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent), ui(new Ui::MainWindow)
{
    ui->setupUi(this); // Setup the UI components
    autoSaveEnabled = false;  // Auto-save is initially disabled
    autoSaveTimer = new QTimer(this);  // Create the auto-save timer
    //darkmode init
    isDarkmode = false;

    connect(autoSaveTimer, &QTimer::timeout, this, &MainWindow::autoSaveDocument); // Connect the timer signal to the auto-save function
    tabWidth = 4;
    useSpacesForTabs = true;

           // Initialize word count label
    wordCountLabel = new QLabel("Words: 0", this);
    statusBar()->addPermanentWidget(wordCountLabel);

           // Get document path
    QString documentsPath = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    QString sessionFilePath = documentsPath + "/NotepadAppSession.ini";

           // Load search history from document
    QSettings settings(sessionFilePath, QSettings::IniFormat);

           // Initialize the tab widget and set it as the central widget
    tabWidget = new QTabWidget(this);
    tabWidget->setTabsClosable(true);
    tabWidget->setMovable(true);  // Enable drag-and-drop rearrangement
    setCentralWidget(tabWidget);

           // Connect the tab close signal
    connect(tabWidget, &QTabWidget::tabCloseRequested, this, &MainWindow::on_tabCloseRequested);

           // Load session data
    int tabCount = settings.value("tabCount", 0).toInt();
    for (int i = 0; i < tabCount; ++i) {
        QString filePath = settings.value(QString("tab%1_filePath").arg(i)).toString();
        QString content = settings.value(QString("tab%1_content").arg(i)).toString();

        QTextEdit *editor = new QTextEdit(this);
        editor->setPlainText(content);
        int tabIndex = tabWidget->addTab(editor, filePath.isEmpty() ? tr("Untitled") : QFileInfo(filePath).fileName());
        tabFileMap[editor] = filePath;
    }

           // Restore the current tab index
    int currentTab = settings.value("currentTab", 0).toInt();
    tabWidget->setCurrentIndex(currentTab);

           // If no tabs were restored, open a new one
    if (tabCount == 0) {
        on_actionNew_triggered();
    }
    //speech init
    speech = new QTextToSpeech(this);
}

// Destructor
MainWindow::~MainWindow()
{
    delete ui; // Cleanup the UI components
}

// Tab Management Functions

// Current editor mapping
QTextEdit* MainWindow::currentEditor()
{
    return qobject_cast<QTextEdit*>(tabWidget->currentWidget());
}

// Handle tab close requests
void MainWindow::on_tabCloseRequested(int index)
{
    QWidget *widget = tabWidget->widget(index);
    if (widget) {
        tabWidget->removeTab(index);
        delete widget;  // Delete the widget to free memory
    }
}

// Document Management Functions

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

                   // Store the file path in tabFileMap
            tabFileMap[editor] = fileName;

            file.close();
        }
    }
}



// Save file action: Saves the current content to the current file
void MainWindow::on_actionSave_triggered()
{
    QTextEdit *editor = currentEditor();
    if (!editor) return;

    QString currentFile = tabFileMap.value(editor);

    if (currentFile.isEmpty()) {
        on_actionSave_As_triggered();
    } else {
        QFile file(currentFile);
        if (!file.open(QIODevice::WriteOnly | QFile::Text)) {
            QMessageBox::warning(this, "Warning", "Cannot save file: " + file.errorString());
            return;
        }
        QTextStream out(&file);
        QString text = editor->toPlainText();
        out << text;
        file.close();

        tabWidget->setTabText(tabWidget->currentIndex(), QFileInfo(currentFile).fileName());
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
        QString text = editor->toPlainText();
        out << text;
        file.close();

               // Update the file path in tabFileMap
        tabFileMap[editor] = fileName;
        tabWidget->setTabText(tabWidget->currentIndex(), QFileInfo(fileName).fileName());
    }
}



//close file
void MainWindow::closeEvent(QCloseEvent *event)
{
    QString documentsPath = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    QString sessionFilePath = documentsPath + "/NotepadAppSession.ini";

    QSettings settings(sessionFilePath, QSettings::IniFormat);

    settings.clear();

    int tabCount = tabWidget->count();
    settings.setValue("tabCount", tabCount);

    for (int i = 0; i < tabCount; ++i) {
        QWidget *widget = tabWidget->widget(i);
        QTextEdit *editor = qobject_cast<QTextEdit *>(widget);

        if (editor) {
            QString filePath = tabFileMap.value(editor, QString());
            settings.setValue(QString("tab%1_filePath").arg(i), filePath);
            settings.setValue(QString("tab%1_content").arg(i), editor->toPlainText());
        }
    }

    settings.setValue("currentTab", tabWidget->currentIndex());

    qDebug() << "Session saved with tab count:" << tabCount;

    QMainWindow::closeEvent(event);
}

//Text-formatting
void MainWindow::on_actionAdd_Bullet_Points_triggered() {
    QTextEdit *editor = currentEditor();
    if (!editor) return;  // Ensure there is a valid text editor

    QTextCursor cursor = editor->textCursor();
    cursor.beginEditBlock();

    QTextListFormat listFormat;
    listFormat.setStyle(QTextListFormat::ListDisc); // Set the style to bullets
    cursor.createList(listFormat); // Create a bullet list

    cursor.endEditBlock();
}

void MainWindow::on_actionAdd_Numberings_triggered() {
    QTextEdit *editor = currentEditor();
    if (!editor) return;  // Ensure there is a valid text editor

    QTextCursor cursor = editor->textCursor();
    cursor.beginEditBlock();

    QTextListFormat listFormat;
    listFormat.setStyle(QTextListFormat::ListDecimal); // Set the style to numbering
    cursor.createList(listFormat); // Create a numbered list

    cursor.endEditBlock();
}



// Auto-Save Functions

// Auto-save trigger
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

// Save interval setting
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

// Auto-save document function
void MainWindow::autoSaveDocument(){
    if (!currentFile.isEmpty()){
        on_actionSave_triggered(); // save the file
    }
}

// Text Editing Functions

// Undo action: Undo the last action in the text editor
void MainWindow::on_actionUndo_triggered()
{
    QTextEdit *editor = currentEditor();
    if (editor) editor->undo();
}

// Redo action: Redo the previously undone action in the text editor
void MainWindow::on_actionRedo_triggered()
{
    QTextEdit *editor = currentEditor();
    if (editor) editor->redo();
}

// Cut action: Cut the selected text
void MainWindow::on_actionCut_triggered()
{
    QTextEdit *editor = currentEditor();
    if (editor) editor->cut();
}

// Copy action: Copy the selected text
void MainWindow::on_actionCopy_triggered()
{
    QTextEdit *editor = currentEditor();
    if (editor) editor->copy();
}

// Paste action: Paste the copied text
void MainWindow::on_actionPaste_triggered()
{
    QTextEdit *editor = currentEditor();
    if (editor) editor->paste();
}

// Select All action: Selects all the text in the editor
void MainWindow::on_actionSelect_All_triggered()
{
    QTextEdit *editor = currentEditor();
    if (editor) editor->selectAll();
}

// Find and Replace Functions

// Find action: Finds the text in the document
void MainWindow::on_actionFind_triggered()
{
    QTextEdit *editor = currentEditor();
    if (!editor) return;

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

    editor->moveCursor(QTextCursor::Start); // Start from the beginning of the document

           // Perform the search
    if (!editor->find(text, options)) {
        QMessageBox::information(this, tr("Find"), tr("Cannot find \"%1\"").arg(text)); // Show message if text not found
        return;
    }

           // Loop to find next/previous occurrences
    while (true) {
        QMessageBox::StandardButton nextReply = QMessageBox::question(this, tr("Find Next"), tr("Do you want to find the next occurrence?"), QMessageBox::Yes|QMessageBox::No|QMessageBox::Cancel);

        if (nextReply == QMessageBox::Yes) {
            if (!editor->find(text, options)) {
                QMessageBox::information(this, tr("Find Next"), tr("No more occurrences of \"%1\"").arg(text));
                break;
            }
        } else if (nextReply == QMessageBox::No) {
            nextReply = QMessageBox::question(this, tr("Find Previous"), tr("Do you want to find the previous occurrence?"), QMessageBox::Yes|QMessageBox::Cancel);

            if (nextReply == QMessageBox::Yes) {
                if (!editor->find(text, options | QTextDocument::FindBackward)) {
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
    QTextEdit *editor = currentEditor();
    if (!editor) return;

           // Prompt the user for the text to find
    bool ok;
    QString findText = QInputDialog::getText(this, tr("Find"), tr("Find what:"), QLineEdit::Normal, "", &ok);

    if (ok && !findText.isEmpty()) {
        // Prompt the user for the replacement text
        QString replaceText = QInputDialog::getText(this, tr("Replace"), tr("Replace with:"), QLineEdit::Normal, "", &ok);
        if (ok) {
            QTextCursor cursor = editor->textCursor(); // Get the current cursor
            QTextDocument *document = editor->document(); // Get the document

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

// Text Formatting Functions

// Highlight action: Opens a color picker dialog to highlight text
void MainWindow::on_actionHighlight_triggered()
{
    QColor color = QColorDialog::getColor(Qt::yellow, this, "Select Highlight Color");

    if (color.isValid()) {
        QTextEdit *editor = currentEditor(); // Make sure this function is valid and returns the current QTextEdit
        if (!editor) return; // If there's no editor, exit early

        QTextCursor cursor = editor->textCursor(); // Get the current text cursor
        if (!cursor.hasSelection()) return; // If no text is selected, exit early

        QTextCharFormat format;
        format.setBackground(color); // Set the background color to the selected color

        cursor.mergeCharFormat(format); // Apply the formatting to the selected text
    }
}

// Highlight predefined colors
void MainWindow::on_actionHighlight_Yellow_triggered()
{
    highlightTextWithColor(Qt::yellow);
}

void MainWindow::on_actionHighlight_Green_triggered()
{
    highlightTextWithColor(Qt::green);
}

void MainWindow::on_actionHighlight_Blue_triggered()
{
    QColor customBlue(0, 0, 255);
    highlightTextWithColor(customBlue);
}

// Helper function: Apply the selected color to the text
void MainWindow::highlightTextWithColor(const QColor &color)
{
    QTextEdit *editor = currentEditor(); // Get the current editor
    if (!editor) return; // If no editor is found, exit early

    QTextCursor cursor = editor->textCursor(); // Get the current text cursor
    if (!cursor.hasSelection()) return; // If no text is selected, exit early

    QTextCharFormat format;
    format.setBackground(color); // Set the background to the specified color
    cursor.mergeCharFormat(format); // Apply the formatting
}

// Clear highlight action: Clears the highlight color
void MainWindow::on_actionClear_Highlight_triggered()
{
    highlightTextWithColor(Qt::transparent); // Clear highlight by setting color to transparent
}

// Text styling actions

// Bold action: Toggles bold formatting on the selected text
void MainWindow::on_actionBold_triggered() {
    QTextEdit *editor = currentEditor();
    if (!editor) return;  // Ensure there is a valid text editor

    QTextCursor cursor = editor->textCursor();

    if (!cursor.hasSelection()) {
        // Apply format to new text or the cursor position
        QTextCharFormat format;
        format.setFontWeight(cursor.charFormat().fontWeight() == QFont::Bold ? QFont::Normal : QFont::Bold);
        editor->setCurrentCharFormat(format);
        return;
    }

    QTextCharFormat format = cursor.charFormat();
    format.setFontWeight(format.fontWeight() == QFont::Bold ? QFont::Normal : QFont::Bold);
    cursor.mergeCharFormat(format);
}

// Italic action: Toggles italic formatting on the selected text
void MainWindow::on_actionItalic_triggered() {
    QTextEdit *editor = currentEditor();
    if (!editor) return;  // Ensure there is a valid text editor

    QTextCursor cursor = editor->textCursor();

    if (!cursor.hasSelection()) {
        QTextCharFormat format;
        format.setFontItalic(!cursor.charFormat().fontItalic());
        editor->setCurrentCharFormat(format);
        return;
    }

    QTextCharFormat format = cursor.charFormat();
    format.setFontItalic(!format.fontItalic());
    cursor.mergeCharFormat(format);
}

// Underline action: Toggles underline formatting on the selected text
void MainWindow::on_actionUnderline_triggered() {
    QTextEdit *editor = currentEditor();
    if (!editor) return;  // Ensure there is a valid text editor

    QTextCursor cursor = editor->textCursor();

    if (!cursor.hasSelection()) {
        QTextCharFormat format;
        format.setFontUnderline(!cursor.charFormat().fontUnderline());
        editor->setCurrentCharFormat(format);
        return;
    }

    QTextCharFormat format = cursor.charFormat();
    format.setFontUnderline(!format.fontUnderline());
    cursor.mergeCharFormat(format);
}

// Strikethrough action: Toggles strikethrough formatting on the selected text
void MainWindow::on_actionStrike_through_triggered() {
    QTextEdit *editor = currentEditor();
    if (!editor) return;  // Ensure there is a valid text editor

    QTextCursor cursor = editor->textCursor();

    if (!cursor.hasSelection()) {
        QTextCharFormat format;
        format.setFontStrikeOut(!cursor.charFormat().fontStrikeOut());
        editor->setCurrentCharFormat(format);
        return;
    }

    QTextCharFormat format = cursor.charFormat();
    format.setFontStrikeOut(!format.fontStrikeOut());
    cursor.mergeCharFormat(format);
}

// Subscript and Superscript actions

void MainWindow::on_actionSub_Script_triggered() {
    QTextEdit *editor = currentEditor();
    if (!editor) return;  // Ensure there is a valid text editor

    QTextCursor cursor = editor->textCursor();

    if (!cursor.hasSelection()) {
        QTextCharFormat format;
        format.setVerticalAlignment(cursor.charFormat().verticalAlignment() == QTextCharFormat::AlignSubScript
                                            ? QTextCharFormat::AlignNormal
                                            : QTextCharFormat::AlignSubScript);
        editor->setCurrentCharFormat(format);
        return;
    }

    QTextCharFormat format = cursor.charFormat();
    format.setVerticalAlignment(format.verticalAlignment() == QTextCharFormat::AlignSubScript
                                        ? QTextCharFormat::AlignNormal
                                        : QTextCharFormat::AlignSubScript);
    cursor.mergeCharFormat(format);
}

void MainWindow::on_actionSuper_Script_triggered() {
    QTextEdit *editor = currentEditor();
    if (!editor) return;  // Ensure there is a valid text editor

    QTextCursor cursor = editor->textCursor();

    if (!cursor.hasSelection()) {
        QTextCharFormat format;
        format.setVerticalAlignment(cursor.charFormat().verticalAlignment() == QTextCharFormat::AlignSuperScript
                                            ? QTextCharFormat::AlignNormal
                                            : QTextCharFormat::AlignSuperScript);
        editor->setCurrentCharFormat(format);
        return;
    }

    QTextCharFormat format = cursor.charFormat();
    format.setVerticalAlignment(format.verticalAlignment() == QTextCharFormat::AlignSuperScript
                                        ? QTextCharFormat::AlignNormal
                                        : QTextCharFormat::AlignSuperScript);
    cursor.mergeCharFormat(format);
}

// Font and Spacing Functions

// Change font style
void MainWindow::on_actionFont_Style_triggered() {
    bool ok;
    QFont font = QFontDialog::getFont(&ok, this);
    if (ok) {
        QTextEdit *editor = currentEditor();
        if (!editor) return;  // Ensure there is a valid text editor

        QTextCursor cursor = editor->textCursor();
        QTextCharFormat format;
        format.setFont(font);
        cursor.mergeCharFormat(format);
    }
}

// Line spacing action
void MainWindow::on_actionSpacing_triggered(){
    QTextEdit *editor = currentEditor();
    if (!editor) return;  // Ensure there is a valid text editor

           // Prompt the user to enter the desired line spacing
    qDebug() << "Line Spacing triggered";
    bool ok;
    double spacing = QInputDialog::getDouble(this, "Line Spacing",
                                             "Enter line spacing (e.g., 1.0 for single, 2.0 for double):",
                                             1.0, 0.5, 4.0, 1, &ok);

    if (ok) { // If the user clicked OK and entered a valid number
        QTextCursor cursor = editor->textCursor(); // Get the current text cursor
        QTextBlockFormat blockFormat = cursor.blockFormat(); // Get the format of the current text block

               // Set the line height to the specified value (proportional to single spacing)
        blockFormat.setLineHeight(spacing * 100, QTextBlockFormat::ProportionalHeight);

               // Apply the new format to the current text block
        cursor.setBlockFormat(blockFormat);
    }
}

// Clear all formatting
void MainWindow::on_actionClear_All_Format_triggered()
{
    QTextEdit *editor = currentEditor();
    if (!editor) return;  // Ensure there is a valid text editor

    QTextCursor cursor = editor->textCursor(); // Get the current text cursor

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

// Color Functions

// Set text color
void MainWindow::on_actionText_Color_triggered()
{
    QTextEdit *editor = currentEditor();
    if (!editor) return;  // Ensure there is a valid text editor

           // Open a color dialog to select a text color
    QColor color = QColorDialog::getColor(editor->textColor(), this, tr("Select Text Color"));

    if (color.isValid()) {
        // Apply the selected text color to the text editor
        editor->setTextColor(color);
    }
}

// Set background color
void MainWindow::on_actionBackground_Color_triggered()
{
    QTextEdit *editor = currentEditor();
    if (!editor) return;  // Ensure there is a valid text editor

           // Open a color dialog to select a background color
    QColor color = QColorDialog::getColor(editor->palette().color(QPalette::Base), this, tr("Select Background Color"));

    if (color.isValid()) {
        // Set the background color of the text editor
        QPalette p = editor->palette();
        p.setColor(QPalette::Base, color); // Set the base color (background)
        editor->setPalette(p);
    }
}

// Toggle dark mode
void MainWindow::on_actionDark_Mode_triggered()
{
    QTextEdit *editor = currentEditor();
    if (!editor) return;  // Ensure there is a valid text editor

    QPalette p = editor->palette();

    if (isDarkmode) {
        // Switch to light mode
        p.setColor(QPalette::Base, Qt::white); // Light background color
        p.setColor(QPalette::Text, Qt::black); // Black text color
        isDarkmode = false; // Update the flag
    } else {
        // Switch to dark mode
        p.setColor(QPalette::Base, QColor(53, 53, 53)); // Dark background color
        p.setColor(QPalette::Text, Qt::white); // White text color
        isDarkmode = true; // Update the flag
    }

    editor->setPalette(p);
}

// Word Count and Alignment Functions

// Update the word count display in the status bar
void MainWindow::updateWordCount()
{
    QTextEdit *editor = currentEditor();
    if (!editor) return;  // Ensure there is a valid text editor

           // Get the text from the editor
    QString text = editor->toPlainText();

           // Calculate the word count
    int wordCount = text.split(QRegularExpression("(\\s|\\n|\\r)+"), Qt::SkipEmptyParts).count();

           // Update the word count label
    wordCountLabel->setText(QString("Words: %1").arg(wordCount));
}

// Alignment actions
void MainWindow::on_actionLeft_triggered() {
    QTextEdit *editor = currentEditor();
    if (editor) editor->setAlignment(Qt::AlignLeft);
}

void MainWindow::on_actionCenter_triggered() {
    QTextEdit *editor = currentEditor();
    if (editor) editor->setAlignment(Qt::AlignCenter);
}

void MainWindow::on_actionRight_triggered() {
    QTextEdit *editor = currentEditor();
    if (editor) editor->setAlignment(Qt::AlignRight);
}

void MainWindow::on_actionJustify_triggered() {
    QTextEdit *editor = currentEditor();
    if (editor) editor->setAlignment(Qt::AlignJustify);
}

// Tab width adjustment
void MainWindow::on_actionTab_Width_triggered()
{
    // Get the current editor
    QTextEdit *editor = currentEditor();
    if (!editor) return;  // Ensure there is a valid text editor

           // Prompt the user to enter the desired tab width
    bool ok;
    int tabWidth = QInputDialog::getInt(this, tr("Tab Width"),
                                        tr("Set tab width (number of spaces):"),
                                        4,  // Default value
                                        1,  // Minimum value
                                        20, // Maximum value
                                        1,  // Step value
                                        &ok);

    if (ok) { // If the user clicked OK and entered a valid number
        // Calculate the tab stop distance in pixels based on the font metrics
        QFontMetricsF metrics(editor->font());
        editor->setTabStopDistance(tabWidth * metrics.horizontalAdvance(' '));

               // Optionally show a message box to confirm the change
        QMessageBox::information(this, tr("Tab Width"), tr("Tab width set to %1 spaces.").arg(tabWidth));
    }
}

//speech function
void MainWindow::on_actionText_To_Speech_triggered()
{
    QTextEdit *editor = currentEditor();
    if (!editor) return;

    QString text = editor->toPlainText();  // Get the text from the current editor
    if (!text.isEmpty()) {
        speech->say(text);  // Use the say() function to speak the text
    }
}

//Insert functions
/*
//Table Insertion
void MainWindow::on_actionInsert_Table_triggered()
{
    bool ok;
    int rows = QInputDialog::getInt(this, tr("Insert Table"), tr("Number of rows:"), 2, 1, 100, 1, &ok);
    if (!ok) return;
    int columns = QInputDialog::getInt(this, tr("Insert Table"), tr("Number of columns:"), 2, 1, 100, 1, &ok);
    if (!ok) return;

    QTextCursor cursor = currentEditor()->textCursor();
    QTextTableFormat tableFormat;
    tableFormat.setBorder(1);
    tableFormat.setCellSpacing(0);
    tableFormat.setCellPadding(4);
    cursor.insertTable(rows, columns, tableFormat);
}

void MainWindow::on_actionInsert_Row_triggered()
{
    QTextCursor cursor = currentEditor()->textCursor();
    QTextTable *table = cursor.currentTable();
    if (table) {
        int row = cursor.currentTable()->cellAt(cursor).row();
        table->insertRows(row + 1, 1);
    }
}

void MainWindow::on_actionDelete_Row_triggered()
{
    QTextCursor cursor = currentEditor()->textCursor();
    QTextTable *table = cursor.currentTable();
    if (table) {
        int row = cursor.currentTable()->cellAt(cursor).row();
        table->removeRows(row, 1);
    }
}

void MainWindow::on_actionInsert_Column_triggered()
{
    QTextCursor cursor = currentEditor()->textCursor();
    QTextTable *table = cursor.currentTable();
    if (table) {
        int column = cursor.currentTable()->cellAt(cursor).column();
        table->insertColumns(column + 1, 1);
    }
}

void MainWindow::on_actionDelete_Column_triggered()
{
    QTextCursor cursor = currentEditor()->textCursor();
    QTextTable *table = cursor.currentTable();
    if (table) {
        int column = cursor.currentTable()->cellAt(cursor).column();
        table->removeColumns(column, 1);
    }
}

*/
// Exit Function

// Exit action: Closes the application
void MainWindow::on_actionExit_triggered()
{
    QApplication::quit();
}

