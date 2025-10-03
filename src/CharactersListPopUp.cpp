#include "CharactersListPopUp.h"
#include <QVBoxLayout>
#include <QPushButton>
#include <QHeaderView>
#include <QInputDialog>

#include "ConfigManager.h"

CharactersListPopUp::CharactersListPopUp(QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle("Characters List");
    resize(300, 400);

    auto* layout = new QVBoxLayout(this);

    m_table = new QTableWidget(this);
    m_table->setColumnCount(1);
    m_table->setHorizontalHeaderLabels({ "Nom" });
    m_table->horizontalHeader()->setStretchLastSection(true);
    m_table->verticalHeader()->setVisible(false);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setEditTriggers(QAbstractItemView::DoubleClicked | QAbstractItemView::SelectedClicked);

    layout->addWidget(m_table);

    auto* btnAdd = new QPushButton("Add", this);
    auto* btnRemove = new QPushButton("Delete", this);
    auto* btnClose = new QPushButton("Close", this);

    layout->addWidget(btnAdd);
    layout->addWidget(btnRemove);
    layout->addWidget(btnClose);

    connect(btnAdd, &QPushButton::clicked, this, &CharactersListPopUp::onAddRow);
    connect(btnRemove, &QPushButton::clicked, this, &CharactersListPopUp::onRemoveRow);
    connect(btnClose, &QPushButton::clicked, this, &QDialog::accept);

    // 🔹 Charger la liste au démarrage
    QStringList characters = ConfigManager::loadCharacters();
    for (const QString& name : characters) {
        int row = m_table->rowCount();
        m_table->insertRow(row);
        m_table->setItem(row, 0, new QTableWidgetItem(name));
    }
}

void CharactersListPopUp::accept() {
    QStringList characters;
    for (int r = 0; r < m_table->rowCount(); ++r) {
        auto* item = m_table->item(r, 0);
        if (item) characters << item->text();
    }

    ConfigManager::saveCharacters(characters);

    QDialog::accept();
}

QStringList CharactersListPopUp::values() const {
    QStringList list;
    for (int row = 0; row < m_table->rowCount(); ++row) {
        auto* item = m_table->item(row, 0);
        if (item) list << item->text();
    }
    return list;
}

void CharactersListPopUp::onAddRow() {
    bool ok;
    QString text = QInputDialog::getText(this, "Add character", "Exact name:", QLineEdit::Normal, "", &ok);
    if (ok && !text.isEmpty()) {
        int row = m_table->rowCount();
        m_table->insertRow(row);
        m_table->setItem(row, 0, new QTableWidgetItem(text));
    }
}

void CharactersListPopUp::onRemoveRow() {
    int row = m_table->currentRow();
    if (row >= 0) {
        m_table->removeRow(row);
    }
}
