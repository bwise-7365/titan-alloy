// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// PformPartyTable: the editable per-party grid of the pform GUI -- one row per
// party, column sections Weight | Position I0.. | Salience I0.. | Utility --
// the Qt adaptation of the exemplar's aligned per-actor DataGridView strips.
// ----------------------------------------------
#ifndef VIMCP_APPS_PFORMPARTYTABLE_HPP
#define VIMCP_APPS_PFORMPARTYTABLE_HPP

#include "pformproblem.hpp"

#include <QTableWidget>

#include <string>
#include <vector>

class QColor;

namespace VIMCP::App {

  // The grid shows parties as ROWS while PformData stores position/salience as
  // issues x parties (D x M); setInstance and instanceFromCells are the ONLY
  // two places that transpose. Weight/Position/Salience cells are editable
  // (the exemplar's workflow: Reset randomizes, the user may hand-tweak, Solve
  // reads the cells); the Utility column is read-only output. Programmatic
  // fills block the itemChanged signal, so the owning window hears only USER
  // edits.
  class PformPartyTable : public QTableWidget
  {
    Q_OBJECT

  public:
    explicit PformPartyTable(QWidget* parent = nullptr);

    // Resize to numParties rows and the Weight|Position|Salience|Utility
    // columns for numIssues, with header/vertical labels. Clears all cells.
    void rebuild(Index numParties, Index numIssues,
                 const std::vector<std::string>& partyLabels,
                 const std::vector<std::string>& issueLabels);

    // Fill the editable cells from the instance (transposing D x M -> rows).
    void setInstance(const PformData& instance);

    // Assemble a PformData from the current cell texts (transposing back).
    // Throws std::invalid_argument naming the offending cell on a parse
    // failure; the caller runs validatePformData for the semantic checks.
    PformData instanceFromCells() const;

    // The read-only Utility column (one value per party) / its cleared state.
    void setUtilities(const VectorXd& utilities);
    void clearUtilities();

    // Mark each issue's controlling party under the deterministic parliament
    // (matching = pformMatching of the deterministic k), and, separately, a
    // selected coalition's pattern (kFreeIssue entries are skipped). Pattern
    // highlights paint over deterministic ones; clearHighlights resets both.
    void highlightDeterministic(const std::vector<Index>& matching);
    void highlightPattern(const std::vector<Index>& pattern);
    void clearHighlights();

  protected:

  private:
    // Column-index arithmetic, in one place: 0 = Weight, then D position
    // columns, then D salience columns, then Utility.
    Index positionColumn(Index d) const;
    Index salienceColumn(Index d) const;
    Index utilityColumn() const;

    // Parse one editable cell as a double; throws std::invalid_argument
    // naming the cell.
    double cellValue(Index row, Index column) const;

    void paintPositionCell(Index issue, Index party, const QColor& color);

    Index issueCount = 0;
  };

} // namespace VIMCP::App

#endif // VIMCP_APPS_PFORMPARTYTABLE_HPP
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
