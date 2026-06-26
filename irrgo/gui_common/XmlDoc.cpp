// Copyright Ben Paul Wise. All Rights Reserved.
#include "XmlDoc.h"

#include <QFile>
#include <QIODevice>
#include <QMessageBox>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>

namespace guicommon {

bool saveXmlDocument(QWidget* parent, const QString& appName, const QString& path,
                     const QString& rootName, const QString& schemaLocation,
                     const std::function<void(QXmlStreamWriter&)>& writeBody) {
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::warning(parent, appName, "Could not open the file for writing.");
        return false;
    }

    QXmlStreamWriter xml(&file);
    xml.setAutoFormatting(true);
    xml.writeStartDocument();
    xml.writeStartElement(rootName);
    xml.writeAttribute("xmlns:xsi", "http://www.w3.org/2001/XMLSchema-instance");
    xml.writeAttribute("xsi:noNamespaceSchemaLocation", schemaLocation);

    writeBody(xml);

    xml.writeEndElement();  // rootName
    xml.writeEndDocument();
    return true;
}

bool loadXmlDocument(QWidget* parent, const QString& appName, const QString& path,
                     const std::function<void(QXmlStreamReader&)>& onNode) {
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QMessageBox::warning(parent, appName, "Could not open the file for reading.");
        return false;
    }

    QXmlStreamReader xml(&file);
    while (!xml.atEnd()) {
        xml.readNext();
        onNode(xml);
    }

    if (xml.hasError()) {
        QMessageBox::warning(parent, appName, "Invalid or unreadable game file.");
        return false;
    }
    return true;
}

}  // namespace guicommon
// Copyright Ben Paul Wise. All Rights Reserved.
