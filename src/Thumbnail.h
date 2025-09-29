#pragma once

#include <QMouseEvent>
#include <QToolButton>

#include "ui_Thumbnail.h"
#include "StreamManager.h"

class QToolButton;

struct pw_core;

class Thumbnail final : public QWidget
{
    Q_OBJECT
public:
    explicit Thumbnail(QWidget* parent,
        pw_core* PipeWireCore,
        PortalStreamInfo* StreamInfo); // Constructor

    signals:
    void onVideoFrameAvailable(const QImage &image);

protected:
    ~Thumbnail() override;

    void mousePressEvent(QMouseEvent* event) override;
    //void mouseReleaseEvent(QMouseEvent* event) override; <- DO NOT USE, DO NOT WORK ON KDE
    void showEvent(QShowEvent* event) override;

private:
    void handleStreamStateChanged(pw_stream_state oldState,pw_stream_state newState, const char* error);
    static void onStreamStateChanged(
        void* data,
        pw_stream_state oldState,
        pw_stream_state newState,
        const char *error)
    {
        auto* self = static_cast<Thumbnail*>(data);
        self->handleStreamStateChanged(oldState, newState, error);
    }

    void handleStreamParamChanged(uint32_t id, const spa_pod *param);
    static void onStreamParamChanged(
        void* data,
        uint32_t id,
        const spa_pod* param)
    {
        auto* self = static_cast<Thumbnail*>(data);
        self->handleStreamParamChanged(id, param);
    }

    void handleStreamAddBuffer(pw_buffer *buffer);
    static void onStreamAddBuffer(
        void* data,
        pw_buffer* buffer)
    {
        auto* self = static_cast<Thumbnail*>(data);
        self->handleStreamAddBuffer(buffer);
    }

    void handleStreamRemoveBuffer(pw_buffer *buffer);
    static void onStreamRemoveBuffer(
        void* data,
        pw_buffer* buffer)
    {
        auto* self = static_cast<Thumbnail*>(data);
        self->handleStreamRemoveBuffer(buffer);
    }

    void handleStreamDestroy();
    static void onStreamDestroy(void* data)
    {
        auto* self = static_cast<Thumbnail*>(data);
        self->handleStreamDestroy();
    }

    void handleStreamProcess();
    static void onStreamProcess(void* data)
    {
        auto* self = static_cast<Thumbnail*>(data);
        self->handleStreamProcess();
    }

    const char *streamStateToStr(pw_stream_state state) const;

    void setCloseButtonPosition() const { m_closeBtn->move(width() - m_closeBtn->width() - 5, 5); }

    Ui_ThumbnailWidget* m_Ui;
    QToolButton* m_closeBtn;

    pw_core* m_PipeWireCore; // Core PipeWire
    pw_properties* m_PipeWireProperties; // Propriétés PipeWire
    pw_stream* m_PipeWireStream; // Stream PipeWire
    spa_hook m_StreamListener; // Catalogue abonnement du Stream PipeWire
    uint8_t m_buffer[1024] = {}; // Buffer vidéo

    PortalStreamInfo* m_StreamInfo; // Info du stream

    uint32_t m_videoWidth; // Largeur du rendu (!= TAILLE DE LA THUMBNAIL)
    uint32_t m_videoHeight; // Hauteur du rendu (!= TAILLE DE LA THUMBNAIL)
};
