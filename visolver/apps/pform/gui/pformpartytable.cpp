// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// PformPartyTable implementation (see pformpartytable.hpp).
// ----------------------------------------------
#include "pformpartytable.hpp"

#include <QColor>
#include <QDoubleValidator>
#include <QHeaderView>
#include <QLineEdit>
#include <QLocale>
#include <QMetaType>
#include <QSignalBlocker>
#include <QString>
#include <QStyledItemDelegate>
#include <QVariant>

#include <stdexcept>

namespace VINCP::App {

  namespace {
    // Flat highlight colors (exemplar-inspired, no better/worse semantics):
    // light blue marks the deterministic parliament's controlling parties,
    // pale turquoise a selected coalition's pinned pattern. The foreground is
    // forced dark so the marks stay readable under a dark theme.
    const QColor kDeterministicColor(196, 222, 255);
    const QColor kPatternColor(175, 238, 238);
    const QColor kHighlightText(0, 0, 0);

    // Shows any double-valued cell to three decimals while the full-precision
    // value stays underneath (Ben, 2026-07-10: readers do not distinguish
    // finer than that; the solver should still get the exact number). Editing
    // opens the full value; a non-numeric entry is stored as text, so it is
    // visible as typed and the Solve-time validation can name the cell.
    class PformValueDelegate : public QStyledItemDelegate
    {
    public:
      using QStyledItemDelegate::QStyledItemDelegate;

      QString
      displayText(const QVariant& value, const QLocale& locale) const override
      {
        if (QMetaType::Double == value.typeId()) {
          return QString::number(value.toDouble(), 'f', 3);
        }
        return QStyledItemDelegate::displayText(value, locale);
      }

      QWidget*
      createEditor(QWidget* parent, const QStyleOptionViewItem&,
                   const QModelIndex&) const override
      {
        QLineEdit* editor = new QLineEdit(parent);
        QDoubleValidator* validator = new QDoubleValidator(editor);
        validator->setLocale(QLocale::c());   // "0.5", never "0,5"
        editor->setValidator(validator);
        return editor;
      }

      void
      setEditorData(QWidget* editor, const QModelIndex& index) const override
      {
        QLineEdit* line = static_cast<QLineEdit*>(editor);
        const QVariant value = index.data(Qt::EditRole);
        line->setText((QMetaType::Double == value.typeId())
                          ? QString::number(value.toDouble(), 'g', 15)
                          : value.toString());
        return;
      }

      void
      setModelData(QWidget* editor, QAbstractItemModel* model,
                   const QModelIndex& index) const override
      {
        const QLineEdit* line = static_cast<const QLineEdit*>(editor);
        bool okP = false;
        const double value = line->text().trimmed().toDouble(&okP);
        if (okP) {
          model->setData(index, value, Qt::EditRole);
        }
        else {
          model->setData(index, line->text(), Qt::EditRole);
        }
        return;
      }

    protected:
    private:
    };
  } // namespace

  PformPartyTable::PformPartyTable(QWidget* parent)
    : QTableWidget(parent)
  {
    setSelectionMode(QAbstractItemView::SingleSelection);
    horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    setItemDelegate(new PformValueDelegate(this));
    return;
  }

  void
  PformPartyTable::rebuild(Index numParties, Index numIssues,
                           const std::vector<std::string>& partyLabels,
                           const std::vector<std::string>& issueLabels)
  {
    const QSignalBlocker blocker(this);
    issueCount = numIssues;
    clear();
    setRowCount(static_cast<int>(numParties));
    setColumnCount(static_cast<int>(2 * numIssues + 2));

    QStringList headers;
    headers << "Weight";
    for (Index d = 0; d < numIssues; ++d) {
      headers << QString("Pos %1").arg(
          QString::fromStdString(issueLabels[static_cast<std::size_t>(d)]));
    }
    for (Index d = 0; d < numIssues; ++d) {
      headers << QString("Sal %1").arg(
          QString::fromStdString(issueLabels[static_cast<std::size_t>(d)]));
    }
    headers << "Utility";
    setHorizontalHeaderLabels(headers);

    QStringList parties;
    for (Index m = 0; m < numParties; ++m) {
      parties << QString::fromStdString(partyLabels[static_cast<std::size_t>(m)]);
    }
    setVerticalHeaderLabels(parties);

    for (int row = 0; row < rowCount(); ++row) {
      for (int column = 0; column < columnCount(); ++column) {
        QTableWidgetItem* item = new QTableWidgetItem(QString());
        if (utilityColumn() == static_cast<Index>(column)) {
          item->setFlags(item->flags() & ~Qt::ItemIsEditable);
        }
        setItem(row, column, item);
      }
    }
    return;
  }

  void
  PformPartyTable::setInstance(const PformData& instance)
  {
    const QSignalBlocker blocker(this);
    const Index M = instance.weight.size();
    const Index D = instance.position.rows();
    for (Index m = 0; m < M; ++m) {
      const int row = static_cast<int>(m);
      // Cells hold the FULL-precision double; PformValueDelegate renders it
      // to three decimals on screen. Solve therefore sees exactly what
      // generate()/readPformGms produced (byte-comparable with pform_cli).
      item(row, 0)->setData(Qt::EditRole, instance.weight(m));
      // The grid shows parties as rows; PformData is issues x parties. This
      // and instanceFromCells are the only two transposition points.
      for (Index d = 0; d < D; ++d) {
        item(row, static_cast<int>(positionColumn(d)))
            ->setData(Qt::EditRole, instance.position(d, m));
        item(row, static_cast<int>(salienceColumn(d)))
            ->setData(Qt::EditRole, instance.salience(d, m));
      }
    }
    return;
  }

  PformData
  PformPartyTable::instanceFromCells() const
  {
    const Index M = static_cast<Index>(rowCount());
    const Index D = issueCount;
    PformData instance;
    instance.weight = VectorXd::Zero(M);
    instance.position = MatrixXd::Zero(D, M);
    instance.salience = MatrixXd::Zero(D, M);
    for (Index m = 0; m < M; ++m) {
      instance.weight(m) = cellValue(m, 0);
      for (Index d = 0; d < D; ++d) {
        instance.position(d, m) = cellValue(m, positionColumn(d));
        instance.salience(d, m) = cellValue(m, salienceColumn(d));
      }
    }
    return instance;
  }

  void
  PformPartyTable::setUtilities(const VectorXd& utilities)
  {
    const QSignalBlocker blocker(this);
    for (Index m = 0; m < utilities.size() && m < rowCount(); ++m) {
      item(static_cast<int>(m), static_cast<int>(utilityColumn()))
          ->setText(QString::number(utilities(m), 'f', 4));
    }
    return;
  }

  void
  PformPartyTable::clearUtilities()
  {
    const QSignalBlocker blocker(this);
    for (int row = 0; row < rowCount(); ++row) {
      item(row, static_cast<int>(utilityColumn()))->setText(QString());
    }
    return;
  }

  void
  PformPartyTable::highlightDeterministic(const std::vector<Index>& matching)
  {
    for (std::size_t d = 0; d < matching.size(); ++d) {
      paintPositionCell(static_cast<Index>(d), matching[d], kDeterministicColor);
    }
    return;
  }

  void
  PformPartyTable::highlightPattern(const std::vector<Index>& pattern)
  {
    for (std::size_t d = 0; d < pattern.size(); ++d) {
      if (kFreeIssue == pattern[d]) {
        continue;
      }
      paintPositionCell(static_cast<Index>(d), pattern[d], kPatternColor);
    }
    return;
  }

  void
  PformPartyTable::clearHighlights()
  {
    const QSignalBlocker blocker(this);
    for (int row = 0; row < rowCount(); ++row) {
      for (Index d = 0; d < issueCount; ++d) {
        QTableWidgetItem* cell = item(row, static_cast<int>(positionColumn(d)));
        cell->setData(Qt::BackgroundRole, QVariant());
        cell->setData(Qt::ForegroundRole, QVariant());
      }
    }
    return;
  }

  Index
  PformPartyTable::positionColumn(Index d) const
  {
    return 1 + d;
  }

  Index
  PformPartyTable::salienceColumn(Index d) const
  {
    return 1 + issueCount + d;
  }

  Index
  PformPartyTable::utilityColumn() const
  {
    return 1 + 2 * issueCount;
  }

  double
  PformPartyTable::cellValue(Index row, Index column) const
  {
    const QTableWidgetItem* cell = item(static_cast<int>(row),
                                        static_cast<int>(column));
    const QVariant value =
        (nullptr == cell) ? QVariant() : cell->data(Qt::EditRole);
    if (QMetaType::Double == value.typeId()) {
      return value.toDouble();   // the full-precision stored number
    }
    // A text remnant: either an empty cell or a non-numeric user entry the
    // delegate preserved as typed. QString::toDouble parses with C-locale
    // semantics, matching the C-printf renderers ("0.5", never "0,5").
    const QString text = value.toString().trimmed();
    bool okP = false;
    const double parsed = text.toDouble(&okP);
    if (!okP) {
      const QString party = verticalHeaderItem(static_cast<int>(row))->text();
      const QString header =
          horizontalHeaderItem(static_cast<int>(column))->text();
      throw std::invalid_argument(
          QString("cell (%1, %2) does not hold a number: \"%3\".")
              .arg(party, header, text)
              .toStdString());
    }
    return parsed;
  }

  void
  PformPartyTable::paintPositionCell(Index issue, Index party,
                                     const QColor& color)
  {
    const QSignalBlocker blocker(this);
    if (party < 0 || rowCount() <= party || issueCount <= issue) {
      return;
    }
    QTableWidgetItem* cell =
        item(static_cast<int>(party), static_cast<int>(positionColumn(issue)));
    cell->setBackground(color);
    cell->setForeground(kHighlightText);
    return;
  }

} // namespace VINCP::App
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
