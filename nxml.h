#ifndef NXML_H
#define NXML_H

// Nxml v1.0 by Neil Cooper 3rd Dec 2019
// Implements an easy-to-use XML reader/writer

#include <string>
#include <map>
#include <vector>

#include <libxml/tree.h>

class Nxml
{
public:

    typedef struct XmlNode
        {
        std::string name;
        std::string value;
        std::vector<XmlNode> children;
        } XmlNode;

    typedef std::vector<XmlNode> XmlTree;

    static void dumpTree( const XmlTree& tree, const unsigned int indentLevel = 0 );

    static void dumpNode( const XmlNode& node, const unsigned int indentLevel = 0, bool recurse = true );

    Nxml( const std::string xmlFileName, const bool useDtdInXmlFile = false );

    // Don't add a default value to the below useDtd parameter, as we need it to
    // be explicit to distinguish which form of constructor to use, since
    //  annoyingly a const char* parameter will match with a bool type too.
    Nxml( const std::string xmlFileName, const std::string xsdFileName, const bool useDtd );

    virtual ~Nxml();

    const XmlTree& getTree();

    void dumpTree( const unsigned int indentLevel = 0 );

protected:
   static void handleLibxmlGenericError(void* validationContext, const char *format, ...);
   static void handleLibxmlStructuredError(void* validationContext, const xmlErrorPtr error );

   xmlDocPtr loadLibxmlDoc( const std::string xmlFileName, const bool useDtdInXmlFile );
   void buildFromLibxmlNode( xmlNode* libXmlNode, XmlTree& tree );
   void loadTreeFromLibxmlTree( const xmlDocPtr libxmlTree, XmlTree& tree );
   void reportUnexpectedContent( xmlNode* libNode );

   void initLibXmlErrorHandling();
   void closeLibxml( xmlDocPtr libxmlTree );

   // Following 3 functions are just convenience functions for internal debugging
   void dumpLibxmlNode( const xmlNode* node, const unsigned int indentLevel );
   void dumpFromLibxmlNode( xmlNode* startNode, const unsigned int indentLevel );
   void dumpLibxmlTree( xmlDocPtr libxmlTree );

   XmlTree m_tree;
};

#endif



