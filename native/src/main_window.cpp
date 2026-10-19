#include "vgeditor/main_window.hpp"

#include <QAction>
#include <QAbstractItemView>
#include <QComboBox>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QKeySequence>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QMimeData>
#include <QPushButton>
#include <QStatusBar>
#include <QSplitter>
#include <QSpinBox>
#include <QTableWidget>
#include <QTextEdit>
#include <QToolBar>
#include <QUrl>
#include <QVBoxLayout>
#include <QWidget>

#include <algorithm>
#include <exception>

namespace vgeditor {

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent),
      patchTable_(new QTableWidget(this)),
      libraryLabel_(new QLabel(tr("Open a VG-88 SysEx library to begin"), this)),
    payloadView_(new QTextEdit(this)),
    midiOutput_(new QComboBox(this)),
    patchName_(new QLineEdit(this)),
    deviceId_(new QSpinBox(this))
{
    setWindowTitle(tr("V-Editor"));
    resize(980, 640);
    setAcceptDrops(true);

    auto* fileMenu = menuBar()->addMenu(tr("&File"));
    auto* openAction = fileMenu->addAction(tr("&Open Library…"));
    openAction->setShortcut(QKeySequence::Open);
    connect(openAction, &QAction::triggered, this, [this] {
        const auto path = QFileDialog::getOpenFileName(this, tr("Open VG-88 Library"), {},
            tr("SysEx libraries (*.syx);;All files (*)"));
        if (!path.isEmpty()) {
            openLibrary(path);
        }
    });

    auto* saveAction = fileMenu->addAction(tr("Save Library &As…"));
    saveAction->setShortcut(QKeySequence::SaveAs);
    connect(saveAction, &QAction::triggered, this, [this] { saveLibraryAs(); });
    fileMenu->addSeparator();
    auto* quitAction = fileMenu->addAction(tr("&Quit"));
    quitAction->setShortcut(QKeySequence::Quit);
    connect(quitAction, &QAction::triggered, this, &QWidget::close);

    auto* midiMenu = menuBar()->addMenu(tr("&MIDI"));
    auto* refreshMidiAction = midiMenu->addAction(tr("Refresh MIDI Outputs"));
    connect(refreshMidiAction, &QAction::triggered,
        this, [this] { refreshMidiDestinations(); });
    auto* sendAction = midiMenu->addAction(tr("Send Selected Patch"));
    connect(sendAction, &QAction::triggered, this, [this] { sendSelectedPatch(); });

    auto* midiToolbar = addToolBar(tr("MIDI"));
    midiToolbar->addWidget(new QLabel(tr("Output"), this));
    midiToolbar->addWidget(midiOutput_);
    midiToolbar->addSeparator();
    midiToolbar->addWidget(new QLabel(tr("Device ID"), this));
    deviceId_->setRange(1, 32);
    deviceId_->setValue(1);
    midiToolbar->addWidget(deviceId_);
    midiToolbar->addAction(sendAction);
    refreshMidiDestinations();

    patchTable_->setColumnCount(2);
    patchTable_->setHorizontalHeaderLabels({tr("Patch"), tr("Name")});
    patchTable_->horizontalHeader()->setStretchLastSection(true);
    patchTable_->verticalHeader()->hide();
    patchTable_->setSelectionBehavior(QAbstractItemView::SelectRows);
    patchTable_->setSelectionMode(QAbstractItemView::SingleSelection);
    patchTable_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    connect(patchTable_, &QTableWidget::itemSelectionChanged,
        this, [this] { showSelectedPatch(); });

    libraryLabel_->setTextInteractionFlags(Qt::TextSelectableByMouse);
    payloadView_->setReadOnly(true);
    payloadView_->setLineWrapMode(QTextEdit::NoWrap);

    auto* rightPane = new QWidget(this);
    auto* rightLayout = new QVBoxLayout(rightPane);
    rightLayout->setContentsMargins(0, 0, 0, 0);
    rightLayout->addWidget(libraryLabel_);
    patchName_->setMaxLength(8);
    patchName_->setPlaceholderText(tr("Patch name (8 characters)"));
    auto* renameButton = new QPushButton(tr("Rename"), rightPane);
    auto* nameLayout = new QHBoxLayout;
    nameLayout->addWidget(patchName_, 1);
    nameLayout->addWidget(renameButton);
    rightLayout->addLayout(nameLayout);
    rightLayout->addWidget(payloadView_, 1);
    connect(renameButton, &QPushButton::clicked,
        this, [this] { renameSelectedPatch(); });
    connect(patchName_, &QLineEdit::returnPressed,
        this, [this] { renameSelectedPatch(); });

    auto* splitter = new QSplitter(this);
    splitter->addWidget(patchTable_);
    splitter->addWidget(rightPane);
    splitter->setStretchFactor(0, 1);
    splitter->setStretchFactor(1, 2);
    setCentralWidget(splitter);
    statusBar()->showMessage(tr("Ready"));
}

void MainWindow::refreshMidiDestinations()
{
    midiOutput_->clear();
    const auto destinations = midiTransport_.destinations();
    for (const auto& destination : destinations) {
        midiOutput_->addItem(QString::fromUtf8(destination.name.c_str()),
            QVariant::fromValue<qulonglong>(destination.endpoint));
    }
    if (destinations.empty()) {
        midiOutput_->addItem(tr("No MIDI outputs found"), QVariant::fromValue<qulonglong>(0));
    }
}

void MainWindow::openLibrary(const QString& path)
{
    try {
        auto loaded = SysexLibrary::load(path.toStdString());
        library_ = std::move(loaded);
        libraryPath_ = path;

        patchTable_->setRowCount(static_cast<int>(library_.patches().size()));
        for (std::size_t i = 0; i < library_.patches().size(); ++i) {
            const auto& patch = library_.patches()[i];
            const auto bank = patch.number / 4 + 1;
            const auto number = patch.number % 4 + 1;
            patchTable_->setItem(static_cast<int>(i), 0,
                new QTableWidgetItem(QStringLiteral("%1 - %2").arg(bank).arg(number)));
            patchTable_->setItem(static_cast<int>(i), 1,
                new QTableWidgetItem(QString::fromLatin1(patch.name.c_str())));
        }

        libraryLabel_->setText(QFileInfo(path).fileName());
        patchTable_->selectRow(0);
        statusBar()->showMessage(tr("%1 patches loaded").arg(library_.patches().size()));
    } catch (const std::exception& error) {
        QMessageBox::critical(this, tr("Unable to Open Library"), QString::fromUtf8(error.what()));
    }
}

void MainWindow::saveLibraryAs()
{
    if (library_.originalBytes().empty()) {
        statusBar()->showMessage(tr("No library is open"));
        return;
    }

    const auto path = QFileDialog::getSaveFileName(this, tr("Save VG-88 Library"), libraryPath_,
        tr("SysEx libraries (*.syx);;All files (*)"));
    if (path.isEmpty()) {
        return;
    }

    const auto bytes = library_.serializedBytes();
    QFile output(path);
    if (!output.open(QIODevice::WriteOnly)
        || output.write(reinterpret_cast<const char*>(bytes.data()),
            static_cast<qint64>(bytes.size())) != static_cast<qint64>(bytes.size())) {
        QMessageBox::critical(this, tr("Unable to Save Library"), output.errorString());
        return;
    }

    library_.markSaved();
    libraryPath_ = path;
    statusBar()->showMessage(tr("Library saved"));
}

void MainWindow::renameSelectedPatch()
{
    const auto selected = patchTable_->selectedItems();
    if (selected.isEmpty()) {
        statusBar()->showMessage(tr("Select a patch to rename"));
        return;
    }

    const auto row = selected.front()->row();
    try {
        const auto name = patchName_->text().toStdString();
        library_.renamePatch(static_cast<std::size_t>(row), name);
        patchTable_->item(row, 1)->setText(QString::fromLatin1(name.c_str()));
        statusBar()->showMessage(tr("Patch name changed; save the library to keep it"));
    } catch (const std::exception& error) {
        QMessageBox::warning(this, tr("Invalid Patch Name"), QString::fromUtf8(error.what()));
    }
}

void MainWindow::sendSelectedPatch()
{
    const auto selected = patchTable_->selectedItems();
    if (selected.isEmpty()) {
        statusBar()->showMessage(tr("Select a patch to send"));
        return;
    }

    const auto row = selected.front()->row();
    const auto endpoint = static_cast<std::uintptr_t>(
        midiOutput_->currentData().toULongLong());
    try {
        midiTransport_.sendPatch(library_.patches().at(static_cast<std::size_t>(row)),
            static_cast<std::uint8_t>(deviceId_->value() - 1), endpoint);
        statusBar()->showMessage(tr("Patch sent"), 3000);
    } catch (const std::exception& error) {
        QMessageBox::critical(this, tr("MIDI Send Failed"), QString::fromUtf8(error.what()));
    }
}

void MainWindow::showSelectedPatch()
{
    const auto selected = patchTable_->selectedItems();
    if (selected.isEmpty()) {
        payloadView_->clear();
        return;
    }

    const auto row = selected.front()->row();
    const auto& patch = library_.patches().at(static_cast<std::size_t>(row));
    patchName_->setText(QString::fromLatin1(patch.name.c_str()));
    QString text;
    for (std::size_t offset = 0; offset < patch.payload.size(); offset += 16) {
        text += QStringLiteral("%1  ").arg(offset, 4, 16, QLatin1Char('0'));
        const auto end = std::min(offset + 16, patch.payload.size());
        for (auto i = offset; i < end; ++i) {
            text += QStringLiteral("%1 ").arg(patch.payload[i], 2, 16, QLatin1Char('0'));
        }
        text += QLatin1Char('\n');
    }
    payloadView_->setPlainText(text);
    statusBar()->showMessage(tr("Patch %1: %2").arg(patch.number + 1).arg(
        QString::fromLatin1(patch.name.c_str())));
}

void MainWindow::dragEnterEvent(QDragEnterEvent* event)
{
    if (event->mimeData()->hasUrls()) {
        event->acceptProposedAction();
    }
}

void MainWindow::dropEvent(QDropEvent* event)
{
    const auto urls = event->mimeData()->urls();
    if (!urls.isEmpty() && urls.front().isLocalFile()) {
        openLibrary(urls.front().toLocalFile());
        event->acceptProposedAction();
    }
}

} // namespace vgeditor