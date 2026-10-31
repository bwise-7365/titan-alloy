// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// A generated map as a hexsheet document, in the form HexKrieg writes (HexRules
// corpus/valid/hexkrieg-save-sheet.xml): one grid whose default terrain is the commonest; a palette
// of the fills; one terrain per setting, with its move-cost; a hexes list for each other terrain;
// hex/@alt where the altitude is not 0; the roads as link kind="road" chains; the seed.
// ----------------------------------------------
#pragma once
#include "hexgen/GenMap.h"
#include "hexgen/Settings.h"

#include "hexxml/SheetDoc.h"

namespace HexGen {

  HexXml::SheetDoc toSheet(const GenMap&, const Settings&);

}  // namespace HexGen
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
