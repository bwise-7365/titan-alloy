// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// The resolution stack in a hexsave document (M6b review): a Position's entries written as
// <resolution><owe>, and the document model copied into the form PositionBuilder reads.
// ----------------------------------------------
#pragma once
#include "hexengine/GameNames.h"
#include "hexmodel/Position.h"
#include "hexrecord/SaveModel.h"
#include "hexxml/SaveDoc.h"

#include <vector>

namespace HexRecord::Detail {

  // Bottom first. A game obligation goes through the codec, which throws when it cannot write it.
  std::vector<SaveOwe> resolutionOf(const HexModel::Position&, const HexEngine::GameNames&,
                                    const HexModel::ObligationCodec&);

  // SaveModel -> SaveDoc: the two mirror the same schema.
  HexXml::SaveOweDoc asOweDoc(const SaveOwe&);

}  // namespace HexRecord::Detail
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
