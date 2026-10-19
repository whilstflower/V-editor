#pragma once

#include "vgeditor/core_midi.hpp"
#include "vgeditor/sysex_library.hpp"

#include <QMainWindow>

class QLabel;
class QComboBox;
class QLineEdit;
class QSpinBox;
class QTableWidget;
class QTextEdit;

namespace vgeditor {

class MainWindow final : public QMainWindow {
public:
    explicit MainWindow(QWidget* parent = nullptr);
    void openLibrary(const QString& path);

protected:
    void dragEnterEvent(QDragEnterEvent* event) override;
    void dropEvent(QDropEvent* event) override;

private:
    void saveLibraryAs();
    void renameSelectedPatch();
    void sendSelectedPatch();
    void showSelectedPatch();
    void refreshMidiDestinations();

    QTableWidget* patchTable_;
    QLabel* libraryLabel_;
    QTextEdit* payloadView_;
    QComboBox* midiOutput_;
    QLineEdit* patchName_;
    QSpinBox* deviceId_;
    MidiTransport midiTransport_;
    SysexLibrary library_;
    QString libraryPath_;
};

} // namespace vgeditor