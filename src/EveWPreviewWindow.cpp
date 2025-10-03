#include "EveWPreviewWindow.h"

#include <qtimer.h>
#include "CharactersListPopUp.h"
#include "ConfigManager.h"
#include "KWinManager.h"
#include "StreamManager.h"
#include "Thumbnail.h"

EveWPreviewWindow::EveWPreviewWindow()
{
    qInfo() << "EveWPreviewWindow::EveWPreviewWindow()";

    setAttribute(Qt::WA_DeleteOnClose);
    m_StreamManager = new StreamManager(this);

    //=== UI
    m_Ui = new Ui_EveWPreviewWindow();
    m_Ui->setupUi(this); // UI deviens Ui_MainWindow
    setFixedSize(400, 200); // Fixe la taille de la fenetre

    const QSize loadedThumbnailsSize = ConfigManager::loadThumbnailsSize(); // Charge la taille des previews
    m_Ui->WidthLineEdit->setText(QString::number(loadedThumbnailsSize.width())); // Applique la taille sur width
    m_Ui->HeightLineEdit->setText(QString::number(loadedThumbnailsSize.height())); // Applique la taille sur height

    const QIntValidator* sizeValidator = new QIntValidator(180, 1920, this); // IntValidator pour les champs 'Size'
    m_Ui->WidthLineEdit->setValidator(sizeValidator);
    m_Ui->HeightLineEdit->setValidator(sizeValidator);

    //= Champ Largeur
    connect( // Abonnement au changement temp réel, uniquement pour les valeurs trop hautes
        m_Ui->WidthLineEdit,
        &QLineEdit::textChanged,
        this,
        [this](const QString &text) {
            if (const int value = text.toInt(); value > 1920) {
                m_Ui->WidthLineEdit->setText("720");}
    });
    connect( // Abonnement à la confirmation, valeurs trop haute et trop basse
    m_Ui->WidthLineEdit,
    &QLineEdit::editingFinished,
    this,
    [this] {
        if (m_Ui->WidthLineEdit->text().toInt() < 180) { // Empêche les valeurs invalides
            m_Ui->WidthLineEdit->setText("180");
        }
        const int width = m_Ui->WidthLineEdit->text().toInt(); // Largeur
        const int height = m_Ui->HeightLineEdit->text().toInt(); // Hauteur
        emit onThumbnailsSizeSettingsChanged(width, height); // Signale le changement de taille
        ConfigManager::saveThumnailsSize(width, height); // Sauvegarde
    });
    //=

    //= Champ Hauteur
    connect( // Abonnement au changement temp réel, uniquement pour les valeurs trop hautes
        m_Ui->HeightLineEdit,
        &QLineEdit::textChanged,
        this,
        [this](const QString &text) {
            bool ok;
            if (const int value = text.toInt(&ok); ok && value > 1920) {
                m_Ui->HeightLineEdit->setText("720");}
    });
    connect( // Abonnement à la confirmation, valeurs trop haute et trop basse
        m_Ui->HeightLineEdit,
        &QLineEdit::editingFinished,
        this,
        [this] {
            if (m_Ui->HeightLineEdit->text().toInt() < 180) {
                m_Ui->HeightLineEdit->setText("180");
            }
            const int width = m_Ui->WidthLineEdit->text().toInt(); // Largeur
            const int height = m_Ui->HeightLineEdit->text().toInt(); // Hauteur
            emit onThumbnailsSizeSettingsChanged(width, height); // Signale le changement de taille
            ConfigManager::saveThumnailsSize(width, height); // Sauvegarde
        }
    );
    //=

    // Connecte le bouton de création des previews
    connect(
        m_Ui->SetupPreviewsButton,
        &QPushButton::clicked,
        this,
        [this] {
            if (m_StreamManager->getScreenCastState() == ScreenCastState::Idle) {
                m_StreamManager->SetupPreviews();
            } else if (m_StreamManager->getScreenCastState() == ScreenCastState::Active) {
                m_StreamManager->ClosePreviews();
            }
        }
    );

    // Connecte le boutton d'édition des characters
    connect(
        m_Ui->EditCharactersList,
        &QPushButton::clicked,
        this,
        [] {
            CharactersListPopUp popup;
            if (popup.exec() == QDialog::Accepted) {
                QStringList list = popup.values();
                for (const QString& str : list) {
                    qDebug() << "->" << str;
                }
            }
        }
    );
}
