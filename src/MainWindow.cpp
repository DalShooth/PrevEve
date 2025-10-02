#include "MainWindow.h"

#include <qevent.h>
#include <qtimer.h>
#include "CharactersListPopUp.h"
#include "ConfigManager.h"
#include "KWinManager.h"
#include "StreamManager.h"
#include "Thumbnail.h"

MainWindow::MainWindow()
{
    qInfo() << "CONSTRUCTOR MainWindow()";

    //=== UI
    m_Ui = new Ui_MainWindow();
    m_Ui->setupUi(this); // UI deviens Ui_MainWindow
    setFixedSize(400, 200); // Fixe la taille de la fenetre

    const QSize loadedThumbnailsSize = ConfigManager::Instance()->loadThumbnailsSize(); // Charge la taille des previews
    m_Ui->WidthLineEdit->setText(QString::number(loadedThumbnailsSize.width())); // Applique la taille sur width
    m_Ui->HeightLineEdit->setText(QString::number(loadedThumbnailsSize.height())); // Applique la taille sur height

    const QIntValidator* sizeValidator = new QIntValidator(180, 720, this); // IntValidator pour les champs 'Size'
    m_Ui->WidthLineEdit->setValidator(sizeValidator);
    m_Ui->HeightLineEdit->setValidator(sizeValidator);

    //= Champ Largeur
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
    //=

    //= Champ Hauteur
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
            if (StreamManager::GetInstance().getScreenCastState() == ScreenCastState::Idle) {
                StreamManager::GetInstance().SetupPreviews();
            } else if (StreamManager::GetInstance().getScreenCastState() == ScreenCastState::Active) {
                StreamManager::GetInstance().ClosePreviews();
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

void MainWindow::onSavePositionButtonClicked()
{
    qInfo() << "onSavePositionButtonClicked()";

    m_Ui->SavePositionsButton->setEnabled(false);

    for (auto* thumbnail: StreamManager::GetInstance().m_ThumbnailsList) {
        qInfo() << thumbnail->pos();
    }

    // connect( // Se connecter au signal de réponse du DBus
    //     KWinManager::Instance(),
    //     &KWinManager::onThumbnailsPositionsReceived,
    //     this,
    //     [](const QList<ThumbnailPosition> &thumbnailsPositions) {
    //         ConfigManager::Instance()->saveThumbnailsPositions(thumbnailsPositions);
    // });
    // KWinManager::GetThumbnailsPositions(); // Éxécute le Script KWin

    QTimer::singleShot(3000, [this] {
        m_Ui->SavePositionsButton->setEnabled(true);
    });
}

