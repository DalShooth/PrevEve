#include "Thumbnail.h"

#include <qtimer.h>
#include <QWindow>
#include <QComboBox>
#include <QPoint>
#include <pipewire/keys.h>
#include <spa/debug/pod.h>
#include <spa/debug/types.h>
#include <spa/param/format-utils.h>
#include <spa/param/video/raw-utils.h>

#include "ConfigManager.h"
#include "KWinManager.h"
#include "MainWindow.h"

Thumbnail::Thumbnail(QWidget* parent,
    pw_core* PipeWireCore,
    PortalStreamInfo* StreamInfo,
    const int ThumbnailId,
    QStringList* characters) :
        QWidget(parent),
        m_thumbnailId(ThumbnailId),
        m_PipeWireCore(PipeWireCore),
        m_StreamInfo(StreamInfo)
{
    qInfo() << "CONSTRUCTOR [Thumbnail]";

    const MainWindow* mainWindow = qobject_cast<MainWindow*>(parent); // Parent MainWindow

    //= UI
    m_Ui = new Ui_ThumbnailWidget;
    m_Ui->setupUi(this); // UI deviens Ui_ThumbnailWidget
    //=

    //= Changement de taille: Maintenant + Dynamique
    resize(mainWindow->GetUi().WidthLineEdit->text().toInt(), mainWindow->GetUi().HeightLineEdit->text().toInt());
    connect(
        mainWindow,
        &MainWindow::onThumbnailsSizeSettingsChanged,
        this,
        [this, mainWindow] {
            resize( // Redimensionner le widget | Il est aussi possible de le faire avec un script KWin
                mainWindow->GetUi().WidthLineEdit->text().toInt(),
                mainWindow->GetUi().HeightLineEdit->text().toInt()
            );
            m_closeBtn->move(width() - m_closeBtn->width() - 5, 5); // Re-positionne le boutton close
            m_characterSelectComboBox->move(5, height() - m_characterSelectComboBox->height() - 5); // Re-positionne le combobox
    });
    //=

    setWindowFlags(Qt::FramelessWindowHint | Qt::SplashScreen);
    qInfo() << "id : " << m_thumbnailId;
    setStyleSheet("background-color: red;");

    //= Bouton de fermeture
    m_closeBtn = new QToolButton(this);
    m_closeBtn->setIcon(style()->standardIcon(QStyle::SP_TitleBarCloseButton));
    m_closeBtn->setFixedSize(20, 20);
    setCloseButtonPosition(); // Positionne le boutton Todo -> peut etre un moyent de l'encrée ?
    m_closeBtn->raise();  // Assure qu'il est au-dessus du QLabel
    connect(m_closeBtn, &QToolButton::clicked, this, &QWidget::close);
    //=

    //= ComboBox de séléction de personnage
    m_characterSelectComboBox = new QComboBox(this);
    m_characterSelectComboBox->setFixedWidth(120);
    m_characterSelectComboBox->move(5, height() - m_characterSelectComboBox->height() - 5); // bas gauche
    m_characterSelectComboBox->addItem("");
    m_characterSelectComboBox->addItems(*characters);
    m_characterSelectComboBox->setCurrentIndex(-1);
    connect(m_characterSelectComboBox, &QComboBox::currentTextChanged,
        this, &Thumbnail::onCharacterSelected);
    //

    //= PipeWire
    // Dictionnaire de propriétés PipeWire qui décrit le flux comme vidéo de capture
    m_PipeWireProperties = pw_properties_new(
        PW_KEY_MEDIA_TYPE, "Video",
        PW_KEY_MEDIA_CATEGORY, "Capture",
        PW_KEY_MEDIA_ROLE, "Application",
        nullptr);

    // Crée la structure PipeWireStream
    m_PipeWireStream = pw_stream_new(m_PipeWireCore, "PrevEveScreenCast", m_PipeWireProperties);
    qInfo() << "[openThumbnailsPipe] -> pw_stream_new renvoie stream =" << m_PipeWireStream;
    if (!m_PipeWireStream) {
        qWarning() << "[CONSTRUCTOR [Thumbnail]] -> m_pwStream invalid";
        return;
    }

    static pw_stream_events PipeWireStreamEvents = {};
    PipeWireStreamEvents.version = PW_VERSION_STREAM_EVENTS;
    PipeWireStreamEvents.state_changed = &Thumbnail::onStreamStateChanged;
    PipeWireStreamEvents.param_changed = &Thumbnail::onStreamParamChanged;
    PipeWireStreamEvents.add_buffer = &Thumbnail::onStreamAddBuffer;
    PipeWireStreamEvents.remove_buffer = &Thumbnail::onStreamRemoveBuffer;
    PipeWireStreamEvents.destroy = &Thumbnail::onStreamDestroy;
    PipeWireStreamEvents.process = &Thumbnail::onStreamProcess;

    pw_stream_add_listener(m_PipeWireStream, &m_StreamListener, &PipeWireStreamEvents, this);

    spa_pod_builder podBuilder = SPA_POD_BUILDER_INIT(m_buffer, sizeof(m_buffer));

    const spa_pod* params[1];
    params[0] = static_cast<const spa_pod *>(spa_pod_builder_add_object(
        &podBuilder,
        SPA_TYPE_OBJECT_Format,
        SPA_PARAM_EnumFormat,
        SPA_FORMAT_mediaType,
        SPA_POD_Id(SPA_MEDIA_TYPE_video),
        SPA_FORMAT_mediaSubtype,
        SPA_POD_Id(SPA_MEDIA_SUBTYPE_raw),
        SPA_FORMAT_VIDEO_format,
        SPA_POD_CHOICE_ENUM_Id(6,
        SPA_VIDEO_FORMAT_BGRA,
        SPA_VIDEO_FORMAT_BGRx,
        SPA_VIDEO_FORMAT_RGBx,
        SPA_VIDEO_FORMAT_RGB,
        SPA_VIDEO_FORMAT_ARGB,
        SPA_VIDEO_FORMAT_YUY2)
    ));

    qInfo() << "[CONSTRUCTOR [Thumbnail]] ->  appel pw_stream_connect (ID cible =" << m_StreamInfo->nodeId << ")";
    int streamConnectResult = pw_stream_connect(
        m_PipeWireStream,
        PW_DIRECTION_INPUT,
        m_StreamInfo->nodeId,
        static_cast<pw_stream_flags>(PW_STREAM_FLAG_AUTOCONNECT | PW_STREAM_FLAG_MAP_BUFFERS),
        params,
        1);
    qInfo() << "[CONSTRUCTOR [Thumbnail]] ->  pw_stream_connect:" << streamConnectResult;
    if (streamConnectResult < 0) {
        qCritical() << "[CONSTRUCTOR [Thumbnail]] -> Stream connect failed";
        return;
    }
    qInfo() << "[CONSTRUCTOR [Thumbnail]] ->  PipeWireStream created";

    // S'abonné au rendu PipeWire (mise à jour frame par frame)
    connect(
        this,
        &Thumbnail::onVideoFrameAvailable,
        this,
        [this](const QImage& image) {
            m_Ui->PreviewLabel->setPixmap(QPixmap::fromImage(image));
    });
    //=
}

Thumbnail::~Thumbnail()
{
    pw_stream_disconnect(m_PipeWireStream);
    pw_stream_destroy(m_PipeWireStream);
}

void Thumbnail::mousePressEvent(QMouseEvent *event) {
    qInfo() << "mousePressEvent()";
    if (event->button() == Qt::RightButton) {
        window()->windowHandle()->startSystemMove();
        event->accept();
    }
    if (event->buttons() & Qt::LeftButton) {
        KWinManager::Instance()->setFocusedClient(m_character);
    }
}

void Thumbnail::onCharacterSelected(const QString& selectedCharacter) {
    if (selectedCharacter.isEmpty()) {
        return;
    }
    qInfo() << "onCharacterSelected() :" << selectedCharacter;
    m_character = selectedCharacter;

    setWindowTitle(QString("Thumbnail-%1").arg(m_character)); // Titre de la fenêtre, sert au script SetWindowPosition

    // Charger la position de la thumbnial du personnage
    QTimer::singleShot(100, [this] {
        const QPoint thumbnailPosition = ConfigManager::Instance()->loadThumbnailPosition(m_character);
        KWinManager::Instance()->SetWindowPosition(m_character, thumbnailPosition);
    });

    m_characterSelectComboBox->setVisible(false);
}

void Thumbnail::handleStreamStateChanged(
    pw_stream_state oldState,
    pw_stream_state newState,
    const char* error)
{
    qInfo() << "[streamEvents.state_changed] -> Stream state changed from "
            << streamStateToStr(oldState) << "to" << streamStateToStr(newState) << (error ? error : "");
}

void Thumbnail::handleStreamParamChanged(
    const uint32_t id,
    const spa_pod* param)
{
    if (!param) {
        qWarning() << "[streamEvents.param_changed] -> param == nullptr";
        return;
    }

    qInfo() << "[streamEvents.param_changed] -> id ="
            << spa_debug_type_find_short_name(spa_type_param, id);

    spa_debug_pod(2, nullptr, param);

    if (id == SPA_PARAM_Format) {
        spa_video_info_raw info = {};
        uint32_t media_type, media_subtype;

        if (spa_format_parse(param, &media_type, &media_subtype) < 0) {
            qWarning() << "[param_changed:Format] -> impossible de parser le format";
            return;
        }

        if (media_type == SPA_MEDIA_TYPE_video &&
            media_subtype == SPA_MEDIA_SUBTYPE_raw) {

            if (spa_format_video_raw_parse(param, &info) < 0) {
                qWarning() << "[param_changed:Format] -> impossible de parser les infos vidéo";
                return;
            }

            m_videoWidth  = info.size.width;
            m_videoHeight = info.size.height;

            qInfo() << "[param_changed:Format] -> dimensions vidéo ="
                    << m_videoWidth << "x" << m_videoHeight;
            }
    }
}

void Thumbnail::handleStreamAddBuffer(
    pw_buffer* buffer)
{
    qInfo() << "[streamEvents.add_buffer] -> Buffer added";
}

void Thumbnail::handleStreamRemoveBuffer(
    pw_buffer* buffer)
{
    qInfo() << "[streamEvents.remove_buffer] -> Buffer removed";
}

void Thumbnail::handleStreamDestroy()
{
    qInfo() << "[streamEvents.destroy] Stream destroyed";
}

void Thumbnail::handleStreamProcess()
{
    //qInfo() << "[streamEvents.process]";

    pw_buffer* buffer = pw_stream_dequeue_buffer(m_PipeWireStream);
    if (!buffer) {
        qWarning() << "[streamEvents.process] -> pw_buffer invalid";
        return;
    }

    spa_buffer* spaBuf = buffer->buffer;
    if (!spaBuf) {
        qWarning() << "[streamEvents.process] -> spa_buffer invalid";
        pw_stream_queue_buffer(m_PipeWireStream, buffer);
        return;
    }

    if (!spaBuf->datas[0].data || !spaBuf->datas[0].chunk) {
        qWarning() << "[streamEvents.process] -> spaBuf->datas[0] invalid";
        pw_stream_queue_buffer(m_PipeWireStream, buffer);
        return;
    }

    void* src = spaBuf->datas[0].data;
    uint32_t size   = spaBuf->datas[0].chunk->size;
    const uint32_t stride = spaBuf->datas[0].chunk->stride;

     // Construire QImage
     const QImage image(
         static_cast<uchar *>(src),
         m_videoWidth,
         m_videoHeight,
         spaBuf->datas[0].chunk->stride,
         QImage::Format_RGB32);

    // Faire une copie pour ne pas dépendre du buffer PipeWire
    const QImage copy = image.scaled(m_Ui->PreviewLabel->size(), Qt::IgnoreAspectRatio, Qt::SmoothTransformation);

    // Émettre le signal Qt
    emit onVideoFrameAvailable(copy);

    // Rendre le buffer à PipeWire
    pw_stream_queue_buffer(m_PipeWireStream, buffer);
}

const char* Thumbnail::streamStateToStr(const pw_stream_state state) const {
    switch (state) {
        case PW_STREAM_STATE_ERROR:        return "ERROR";
        case PW_STREAM_STATE_UNCONNECTED:  return "UNCONNECTED";
        case PW_STREAM_STATE_CONNECTING:   return "CONNECTING";
        case PW_STREAM_STATE_PAUSED:       return "PAUSED";
        case PW_STREAM_STATE_STREAMING:    return "STREAMING";
        default:                           return "UNKNOWN";
    }
}
