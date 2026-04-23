/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */
package groupw.Network;

import org.xml.sax.ErrorHandler;
import org.xml.sax.SAXException;
import org.xml.sax.SAXParseException;

/**
 * Error Handler for DTD validation of files, e.g. railroad data in XML format.
 *
 * @author BenWise
 */
public class DTDErrorHandler implements ErrorHandler {

    public DTDErrorHandler() {
        dtdValidationFailed = false;
    }
    public boolean dtdValidationFailed;

    @Override
    public void warning(SAXParseException e) throws SAXException {
        System.out.println("WARNING : " + e.getMessage()); // do nothing
        dtdValidationFailed = true;
    }

    @Override
    public void error(SAXParseException e) throws SAXException {
        System.out.println("ERROR : " + e.getMessage());
        dtdValidationFailed = true;
        throw e;
    }

    @Override
    public void fatalError(SAXParseException e) throws SAXException {
        System.out.println("FATAL : " + e.getMessage());
        dtdValidationFailed = true;
        throw e;
    }
}

// =============================================================================
