#include "MainWindow.h"

#include <qevent.h>
#include <qtimer.h>
#include "ThumbnailsProfilesListPopUp.h"
#include "ConfigManager.h"
#include "KWinManager.h"
#include "StreamManager.h"

MainWindow::MainWindow()
{
    qInfo() << "CONSTRUCTOR [MainWindow]";

    //=== UI
    m_Ui = new Ui_MainWindow();
    m_Ui->setupUi(this); // UI deviens Ui_MainWindow
    setFixedSize(400, 200); // Fixe la taille de la fenetre

    // Charger la taille depuis .conf
    const QSize loadedThumbnailsSize = ConfigManager::Instance()->loadThumbnailsSize();
    m_Ui->WidthLineEdit->setText(QString::number(loadedThumbnailsSize.width()));
    m_Ui->HeightLineEdit->setText(QString::number(loadedThumbnailsSize.height()));

    m_SizeValidator = new QIntValidator(180, 720, this); // IntValidator pour les champs 'Size'

    //= Champ Largeur
    m_Ui->WidthLineEdit->setValidator(m_SizeValidator);
    connect( // Abonnement au changement temp réel, uniquement pour les valeurs trop hautes
        m_Ui->WidthLineEdit,
        &QLineEdit::textChanged,
        this,
        [this](const QString &text) {
            if (const int value = text.toInt(); value > 720) {
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
        ConfigManager::Instance()->saveThumnailsSize(width, height); // Sauvegarde
    });
    //

    //= Champ Hauteur
    m_Ui->HeightLineEdit->setValidator(m_SizeValidator);
    connect( // Abonnement au changement temp réel, uniquement pour les valeurs trop hautes
        m_Ui->HeightLineEdit,
        &QLineEdit::textChanged,
        this,
        [this](const QString &text) {
            bool ok;
            if (const int value = text.toInt(&ok); ok && value > 720) {
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
            ConfigManager::Instance()->saveThumnailsSize(width, height); // Sauvegarde
        }
    );
    //=

    // Connecte le bouton de création des previews
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
        }
    );

    // Connecte de bouton de sauvegarde des positions des thumbnails
    connect(
        m_Ui->SavePositionsButton,
        &QPushButton::clicked,
        this,
        &MainWindow::onSavePositionButtonClicked
    );

    // Connecte le boutton d'édition des characters
    connect(
        m_Ui->EditCharactersList,
        &QPushButton::clicked,
        this,
        [] {
            ThumbnailsProfilesListPopUp popup;
            if (popup.exec() == QDialog::Accepted) {
                QStringList list = popup.values();
                for (const QString& str : list) {
                    qDebug() << "->" << str;
                }
            }
        }
    );
}

void MainWindow::onSavePositionButtonClicked()
{
    qInfo() << "[onSavePositionButtonClicked]";

    m_Ui->SavePositionsButton->setEnabled(false);

    connect( // Se connecter au signal de réponse du DBus
        KWinManager::Instance(),
        &KWinManager::onThumbnailsPositionsReceived,
        this,
        [](const QList<ThumbnailPosition> &thumbnailsPositions) {
            ConfigManager::Instance()->saveThumbnailsPositions(thumbnailsPositions);
    });
    KWinManager::Instance()->GetThumbnailsPositions(); // Éxécute le Script KWin

    QTimer::singleShot(3000, [this] {
        m_Ui->SavePositionsButton->setEnabled(true);
    });
}

