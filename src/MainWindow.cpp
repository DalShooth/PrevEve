#include "MainWindow.h"

#include <QValidator>

MainWindow::MainWindow()
{
    qInfo() << "CONSTRUCTOR [MainWindow]";

    //=== UI
    m_Ui = new Ui_MainWindow();
    m_Ui->setupUi(this); // UI deviens Ui_MainWindow
    setFixedSize(400, 200); // Fixe la taille de la fenetre

    m_SizeValidator = new QIntValidator(0, 720, this); // IntValidator pour les champs 'Size'

    // Faire du champ 'Largeur' uniquement un champ Int entre 180 et 720
    m_Ui->WidthLineEdit->setValidator(m_SizeValidator);
    connect(
        m_Ui->WidthLineEdit,
        &QLineEdit::textChanged,
        this,
        [this](const QString &text) {
            bool ok;
            if (const int value = text.toInt(&ok); ok && value > 720) {
                m_Ui->WidthLineEdit->setText("720");}
    });
    connect(
    m_Ui->WidthLineEdit,
    &QLineEdit::editingFinished,
    this,
    [this] {
        if (m_Ui->WidthLineEdit->text().toInt() < 180) {
            m_Ui->WidthLineEdit->setText("180");
        }
        emit onThumbnailsSizeSettingsChanged(
            m_Ui->WidthLineEdit->text().toInt(),
            m_Ui->HeightLineEdit->text().toInt());
    });

    //= Faire du champ 'Hauteur' uniquement un champ Int avec comme maximum 720
    m_Ui->HeightLineEdit->setValidator(m_SizeValidator);
    connect(
        m_Ui->HeightLineEdit,
        &QLineEdit::textChanged,
        this,
        [this](const QString &text) {
            bool ok;
            if (const int value = text.toInt(&ok); ok && value > 720) {
                m_Ui->HeightLineEdit->setText("720");}
    });
    connect(
        m_Ui->HeightLineEdit,
        &QLineEdit::editingFinished,
        this,
        [this] {
            if (m_Ui->HeightLineEdit->text().toInt() < 180) {
                m_Ui->HeightLineEdit->setText("180");
            }
            emit onThumbnailsSizeSettingsChanged(
                m_Ui->WidthLineEdit->text().toInt(),
                m_Ui->HeightLineEdit->text().toInt());
        });
    //===

    // Connecte le bouton test
    connect(
        m_Ui->SetupPreviewsButton,
        &QPushButton::clicked,
        this,
        [] {
            qInfo() << "SetupPreviewsButton clicked";
            if (StreamManager::Instance().getScreenCastState() == ScreenCastState::Idle) {
                StreamManager::Instance().SetupPreviews();
            } else if (StreamManager::Instance().getScreenCastState() == ScreenCastState::Active) {
                StreamManager::Instance().ClosePreviews();
            }
    });
}
