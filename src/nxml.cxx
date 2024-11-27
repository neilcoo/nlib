// nxml.cxx by Neil Cooper. See nxml.h for documentation
#include "nxml.h"

#include <sstream>
#include <libxml/xmlschemastypes.h>

#include "nerror.h"


using namespace std;

static const unsigned short DUMP_INDENT_INCREMENT = 3;


Nxml::Nxml( const string xmlFileName, const bool useDtdInXmlFile )
{
    initLibXmlErrorHandling();
    xmlDocPtr libxmlTree = loadLibxmlDoc( xmlFileName, useDtdInXmlFile );
    loadTreeFromLibxmlTree( libxmlTree, m_tree );
    closeLibxml( libxmlTree );
}


Nxml::Nxml( const string xmlFileName, const string xsdFileName, const bool useDtdInXmlFile )
{
    initLibXmlErrorHandling();

    // Create an XML Schemas parse context for that file/resource expected to contain an XML Schemas file.
    xmlSchemaParserCtxtPtr pctxt = xmlSchemaNewParserCtxt( xsdFileName.c_str() );
    if ( !pctxt )
        ERROR( "Can't create parse context for '", xsdFileName, "'" );

    // parse a schema definition resource and build an internal XML Schema struture which can be used to validate instances.
    xmlSchemaPtr schema = xmlSchemaParse( pctxt );
    xmlSchemaFreeParserCtxt( pctxt );
    if ( !schema )
        ERROR( "Can't create schema definition from '", xsdFileName, "'" );

    xmlDocPtr libxmlTree = loadLibxmlDoc( xmlFileName, useDtdInXmlFile );

    // Create an XML Schemas validation context based on the given schema.
    xmlSchemaValidCtxtPtr ctxt = xmlSchemaNewValidCtxt( schema );
    if ( !ctxt )
        ERROR( "Can't create schema validation context for '", xsdFileName, "'" );

    // Set the error and warning callback informations
    xmlSchemaSetValidErrors(    ctxt,
                                (xmlSchemaValidityErrorFunc)handleLibxmlGenericError,
                                (xmlSchemaValidityWarningFunc)handleLibxmlGenericError,
                                stderr ); // void* ctx    functions context

    // Validate a document tree in memory.
    int ret = xmlSchemaValidateDoc( ctxt, libxmlTree );
    if ( ret < 0 )
        ERROR( "Libxml internal error. xmlSchemaValidateDoc returned ", ret );

    if ( ret > 0 )
        ERROR( "XML file '", xmlFileName, "' did not validate against XSD file '", xsdFileName, "'" );

    loadTreeFromLibxmlTree( libxmlTree, m_tree );
    closeLibxml( libxmlTree );
}


Nxml::~Nxml()
{
}


const Nxml::XmlTree& Nxml::getTree()
{
    return m_tree;
}


void Nxml::dumpNode( const XmlNode& node, const unsigned int indentLevel, bool recurse)
{
    string indent( indentLevel, ' ' );
    cout << indent << node.name;
    if ( !node.value.empty() )
        cout << ": '" << node.value << "'";
    cout << endl;
    if ( recurse && node.children.size() > 0 )
        dumpTree( node.children, indentLevel + DUMP_INDENT_INCREMENT );
}


void Nxml::dumpTree( const XmlTree& tree, const unsigned int indentLevel )
{
    for ( size_t i = 0; i < tree.size(); i++ )
        dumpNode( tree[i], indentLevel, true );
}


void Nxml::dumpTree( const unsigned int indentLevel )
{
    dumpTree( m_tree, indentLevel );
}


// -----------------------------------------------------------------------------

void Nxml::handleLibxmlGenericError(void* validationContext, const char *format, ...)
{
    char* errMsg;
    va_list args;
    va_start( args, format );
    if ( vasprintf( &errMsg, format, args ) == -1 )
        ERROR( "vasprintf() failed! Memory allocation not possible?" );
    va_end( args );

    std::string msg = "Generic Error: ";
    msg += errMsg;
    free( errMsg );
    ERROR( msg );
}


void Nxml::handleLibxmlStructuredError(void* validationContext, const xmlErrorPtr error )
{
    ostringstream msg;
    if ( error->file )
        msg << "File: '" << error->file << "' ";

    if ( error->line != 0 )
        msg << "line " << error->line << ": ";

    msg << error->message;

    if ( error->str1 )
        msg << " " << error->str1;

    if ( error->str2 )
        msg << " " << error->str2;

    if ( error->str3 )
        msg << " " << error->str3;

    ERROR( msg.str() );
}


xmlDocPtr Nxml::loadLibxmlDoc( const std::string xmlFileName, const bool useDtdInXmlFile )
{
    xmlDocPtr libxmlTree = xmlReadFile( xmlFileName.c_str(),
                                        NULL,                // encoding
                                        XML_PARSE_NOBLANKS | // options
                                        ( useDtdInXmlFile ? XML_PARSE_DTDVALID : 0 ) );
    if ( !libxmlTree )
        ERROR( "Failed to parse ", xmlFileName );

    return libxmlTree;
}


void Nxml::buildFromLibxmlNode( xmlNode* libXmlNode, XmlTree& tree )
{
    xmlNode* libNode = libXmlNode;
    if ( libNode->type != XML_COMMENT_NODE )
        {
        if ( libNode->type != XML_ELEMENT_NODE )
            reportUnexpectedContent( libNode );

        XmlNode newNode;
        newNode.name = (const char*)libNode->name;
        xmlNode* child = libNode->children;
        while( child )
            {
            switch( child->type )
                {
                case XML_TEXT_NODE:
                    newNode.value = (char*)child->content;
                    break;

                case XML_ELEMENT_NODE:
                    buildFromLibxmlNode( child, newNode.children );
                    break;

                case XML_COMMENT_NODE:
                    break;

                default:
                    reportUnexpectedContent( child );
                }
            child = child->next;
            }
        tree.push_back( newNode );
        }
}


void Nxml::loadTreeFromLibxmlTree( const xmlDocPtr libxmlTree, XmlTree& tree )
{
    xmlNode* node = xmlDocGetRootElement( libxmlTree );
    while( node )
        {
        buildFromLibxmlNode( xmlDocGetRootElement( libxmlTree ), tree );
        node=node->next;
        }
}


void Nxml::reportUnexpectedContent( xmlNode* libNode )
{
    ERROR("Line ", libNode->line, ": Unexpected content: '", libNode->content, "'");
}


void Nxml::initLibXmlErrorHandling()
{
    xmlSetGenericErrorFunc( NULL, handleLibxmlGenericError );
    xmlSetStructuredErrorFunc( NULL, handleLibxmlStructuredError );
    xmlLineNumbersDefault( 1 );
}


void Nxml::closeLibxml( xmlDocPtr libxmlTree )
{
    if ( libxmlTree )
        xmlFreeDoc( libxmlTree );

    xmlCleanupParser();
    xmlMemoryDump(); // this is to debug memory for regression tests
}


void Nxml::dumpLibxmlNode( const xmlNode* node, const unsigned int indentLevel )
{
    string indent( indentLevel, ' ' );
    cout << indent;

    switch ( node->type )
        {
        case XML_ELEMENT_NODE:
            cout << "Element: '" << node->name << "'"; // never has content
            break;

        case XML_TEXT_NODE:
            cout << "Text: '" << node->content << "'";
            break;

        case XML_COMMENT_NODE:
            cout << "COMMENT: '" << node->content << "'";
            break;

        default:
            cout << "UNHANDLED TYPE: type: " << node->type << " Name '" << node->name << "'" << " Content: '" << node->content << "'";
        }

    cout << endl;
}


void Nxml::dumpFromLibxmlNode( xmlNode* startNode, const unsigned int indentLevel )
{
    xmlNode* node = startNode;
    while ( node )
        {
        dumpLibxmlNode( node, indentLevel );
        if ( node->children )
            dumpFromLibxmlNode( node->children, indentLevel + DUMP_INDENT_INCREMENT  );
        node = node->next;
        }
}


void Nxml::dumpLibxmlTree( xmlDocPtr libxmlTree )
{
    dumpFromLibxmlNode( xmlDocGetRootElement( libxmlTree ), 0 );
}

