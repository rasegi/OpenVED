#pragma once

#include "vec_math_base.h"

#include <QByteArray>
#include <QMainWindow>

#include <memory>
#include <vector>

class QAction;
class QCheckBox;
class QCloseEvent;
class QComboBox;
class QDockWidget;
class QDoubleSpinBox;
class QLabel;
class QLineEdit;
class QSplitter;
class QPlainTextEdit;
class QSpinBox;
class QToolBar;
class QVedWidget;
class TDVecEditCad;
class TDVecFont;
class IVecFontProvider;
class TDFontManager;
class TDVecModel;
class TDViewOperationManager;
struct VEDDocumentViewState;
struct VEDFontResolutionResult;

class MainWindow final : public QMainWindow {
public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override;

protected:
    void closeEvent(QCloseEvent* event) override;

private:
    void createMenus();
    void createToolBars();
    void newDocument();
    void openDocument();
    bool saveDocument();
    bool confirmDiscardUnsavedChanges();
    QAction* addToolAction(
        QToolBar* toolbar,
        const QString& text,
        const QString& iconName,
        bool checkable = false,
        const QString& toolTip = QString());
    void readSettings();
    void writeSettings() const;
    void resetWindowLayout();
    void initializeEditor();
    void installModel(std::unique_ptr<TDVecModel> model, const VEDDocumentViewState* viewState = nullptr);
    void showFontResolutionWarnings(const VEDFontResolutionResult& fontResolution);
    void syncViewActionsFromCanvas();
    void syncUiWithActiveOperation();
    void updateGroupCommandActions();
    void updateModifyCommandActions();
    void updateClipboardCommandActions();
    void updateHistoryCommandActions();
    void updateMouseToleranceCrossCursor();
    void deactivateViewTools();
    void setActiveOperationStatus(const QString& operationName);
    void updateCoordinateStatus(TDMatPoint point, bool valid);
    void updatePageFormatStatus();
    void onDocumentSetup();
    void applyDocumentSettings(const struct TDVecDocumentSettings& settings);
    void exportPdf();
    void printDocument();
    void createCurveDock();
    void showPolyCurveDock();
    void applyPolyCurveSettings();
    void cancelCurveDockOperation();
    void hideCurveDockIfInactive();
    void createRoundRectangleDock();
    void showRoundRectangleDock();
    void applyRoundRectangleSettings();
    void cancelRoundRectangleDockOperation();
    void refreshRoundRectangleDockFromExtVar();
    void hideRoundRectangleDockIfInactive();
    void createRotateDock();
    void showRotateDock();
    void applyRotateSettings();
    void cancelRotateDockOperation();
    void refreshRotateDockFromExtVar();
    bool updateRotateExtVarFromDock(bool showMessage);
    bool cancelDialogOperationFromRightClick();
    void createTextDock();
    void showTextDockForCreate(bool frameText);
    void showTextDockForModify();
    void applyTextSettings();
    bool updateCreateTextExtVar(bool showMessage);
    void cancelTextDockOperation();
    void hideTextDockIfInactive();
    void startFrameTextBoxCreate();
    bool loadDefaultVecFont();
    void populateTextFontCombo();
    const TDVecFont* currentTextVecFont();
    void setTextFontComboToFontName(const char* fontName);

    enum class TextDockMode {
        None,
        CreateText,
        CreateFrameTextBox,
        Modify
    };

    QVedWidget* canvas_;
    std::unique_ptr<TDVecModel> model_;
    std::unique_ptr<TDVecEditCad> editor_;
    std::unique_ptr<TDViewOperationManager> operationManager_;
    QAction* selectAction_;
    QAction* cutAction_;
    QAction* copyAction_;
    QAction* pasteAction_;
    QAction* undoAction_;
    QAction* redoAction_;
    QAction* makeGroupAction_;
    QAction* resolveGroupAction_;
    std::vector<QAction*> alignActions_;
    QAction* lockObjectAction_;
    QAction* moveObjectAction_;
    QAction* moveNodeAction_;
    QAction* insertNodeAction_;
    QAction* deleteNodeAction_;
    QAction* moveBsplineControlPointAction_;
    QAction* rotateObjectAction_;
    QAction* scaleObjectAction_;
    QAction* changeEdgeRoundNodeAction_;
    QAction* modifyCurveAttributeAction_;
    QAction* createTextAction_;
    QAction* createFrameTextAction_;
    QAction* modifyTextAction_;
    QAction* zoomAction_;
    QAction* panAction_;
    QAction* showGridAction_;
    QAction* gridLockAction_;
    QAction* showRulersAction_;
    QAction* mouseToleranceCrossAction_;
    QSplitter* statusBarSplitter_;
    QLabel* activeOperationLabel_;
    QLabel* pageFormatLabel_;
    QLabel* coordinateLabel_;
    QDockWidget* curveDock_;
    QCheckBox* curveShowPolygonCheck_;
    QCheckBox* curveShowControlsCheck_;
    QCheckBox* curveCopyCheck_;
    QLabel* curveDegreeLabel_;
    QLineEdit* curveDegreeEdit_;
    QLineEdit* curveResolutionEdit_;
    QDockWidget* roundRectangleDock_;
    QLineEdit* roundRectangleRadiusEdit_;
    QLineEdit* roundRectangleResolutionEdit_;
    QCheckBox* roundRectangleShowPolygonCheck_;
    QCheckBox* roundRectangleShowControlsCheck_;
    QLabel* roundRectangleWidthLabel_;
    QLabel* roundRectangleHeightLabel_;
    QDockWidget* rotateDock_;
    QLineEdit* rotateAngleEdit_;
    QCheckBox* rotateCopyCheck_;
    QCheckBox* rotateSelectCopyCheck_;
    QLabel* rotateObjectCountLabel_;
    QLabel* rotatePivotLabel_;
    QDockWidget* textDock_;
    QPlainTextEdit* textEdit_;
    QDoubleSpinBox* textXScaleSpin_;
    QDoubleSpinBox* textYScaleSpin_;
    QDoubleSpinBox* textAngleSpin_;
    QDoubleSpinBox* textInclineSpin_;
    QDoubleSpinBox* textLineSpacingSpin_;
    QDoubleSpinBox* textCharSpacingSpin_;
    QSpinBox* textResolutionSpin_;
    QComboBox* textFontCombo_;
    QComboBox* textJustificationCombo_;
    QComboBox* textVerticalAlignCombo_;
    QCheckBox* textVerticalCheck_;
    QCheckBox* textUnderlineCheck_;
    QCheckBox* textScaleDependencyCheck_;
    TextDockMode textDockMode_;
    std::unique_ptr<TDFontManager> fontManager_;
    std::vector<std::unique_ptr<IVecFontProvider>> fontProviders_;
    QByteArray defaultWindowState_;
    QString currentDocumentPath_;
    QString lastDocumentDirectory_;
};
