#pragma once

#include <QDialog>
#include <QTableWidget>

class CharactersListPopUp final : public QDialog {
    Q_OBJECT
public:
    explicit CharactersListPopUp(QWidget* parent = nullptr);

    void accept() override;

    QStringList values() const;  // <-- bien déclarer ici

private slots:
    void onAddRow();
    void onRemoveRow();

private:
    QTableWidget* m_table;
};
