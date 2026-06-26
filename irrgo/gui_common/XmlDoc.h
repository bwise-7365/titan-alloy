// Copyright Ben Paul Wise. All Rights Reserved.
#pragma once
#include <QString>
#include <functional>

class QWidget;
class QXmlStreamWriter;
class QXmlStreamReader;

namespace guicommon {

// Save/load envelope shared by the game GUIs (irrgo, latrunculi, ...). Each game's
// board schema differs, but the surrounding mechanics are identical and live here:
// opening the file, the XML prolog, the root element with its xsi schema-location
// attributes, the read loop, and uniform "could not open" / "invalid file" warning
// dialogs. The game supplies only the body via a callback. See CLAUDE.md
// ("reuse over duplicate").

// Writes a full XML document to `path`: the prolog, a <rootName> element carrying the
// standard xsi noNamespaceSchemaLocation=`schemaLocation` attributes, the game-specific
// content emitted by `writeBody`, then the closing tags. Returns false and shows a
// warning dialog titled `appName` (parented to `parent`, which may be null) when the
// file cannot be opened for writing.
bool saveXmlDocument(QWidget* parent, const QString& appName, const QString& path,
                     const QString& rootName, const QString& schemaLocation,
                     const std::function<void(QXmlStreamWriter&)>& writeBody);

// Opens `path` for reading and drives a QXmlStreamReader, invoking `onNode(reader)`
// after every token read so the caller can dispatch on start/end elements and
// accumulate state in its own closure. Returns false (with an `appName` warning
// dialog) when the file cannot be opened or the stream reports a parse error. A true
// return means the XML was well-formed only; the caller still validates the content
// and reconstructs the game.
bool loadXmlDocument(QWidget* parent, const QString& appName, const QString& path,
                     const std::function<void(QXmlStreamReader&)>& onNode);

}  // namespace guicommon
// Copyright Ben Paul Wise. All Rights Reserved.
