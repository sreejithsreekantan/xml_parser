/*=============================================================================
 
 Developed by Sreejith Sreekantan
 
 You are here by allowed to freely use this code as such or modified.. 
 I have no problem with that, provided you share the knowledge you gain 
 with others as well... 
 
 :-)
 
 ==============================================================================*/
#include <boost/config/warning_disable.hpp>
#include <boost/spirit/include/qi.hpp>
#include <boost/spirit/include/phoenix_core.hpp>
#include <boost/spirit/include/phoenix_operator.hpp>
#include <boost/spirit/include/phoenix_fusion.hpp>
#include <boost/spirit/include/phoenix_stl.hpp>
#include <boost/fusion/include/adapt_struct.hpp>
#include <boost/variant/recursive_variant.hpp>
#include <boost/foreach.hpp>

#include <iostream>
#include <fstream>
#include <string>
#include <vector>

namespace client
{
    namespace fusion = boost::fusion;
    namespace phoenix = boost::phoenix;
    namespace qi = boost::spirit::qi;
    namespace ascii = boost::spirit::ascii;
    
    struct xml_node;
    
    typedef boost::variant< boost::recursive_wrapper<xml_node>, std::string > xml_child_node;
    
    struct xml_node
    {
        std::string name;
        std::vector<xml_child_node> children;
    };
    
}

BOOST_FUSION_ADAPT_STRUCT(
                          client::xml_node,
                          (std::string,name)
                          (std::vector<client::xml_child_node>,children)
                          )

namespace client {
    
    ///////////////////////////////////////////////////////////////////////////
    //  Visitors to print out xml in heirarchy
    ///////////////////////////////////////////////////////////////////////////
    
    int const _tabsize = 4;
    void tab(int indent)
    {
        for (int i=0; i<indent; i++) {
            std::cout<<" ";
        }
    }
    
    struct xml_child_node_printer;
    
    struct xml_node_printer : boost::static_visitor<>{
        int indent;
        xml_node_printer(int indent=0):indent(indent){
        }
        void operator()(xml_node const& xn) const;
    };
    
    struct xml_child_node_printer : boost::static_visitor<>{
        int indent;
        xml_child_node_printer(int indent=0):indent(indent){
        }
        void operator()(std::string const& text) const
        {
            tab(_tabsize+indent);
            std::cout << "text: " << text << std::endl;
        }
        void operator()(client::xml_node const& xn) const
        {
            xml_node_printer(_tabsize+indent)(xn);
        }
    };
    
    void xml_node_printer::operator()(xml_node const& xn) const
    {
        tab(indent);
        std::cout << "tag: " << xn.name << std::endl;
        tab(indent);
        std::cout<< "{"<<std::endl;
        BOOST_FOREACH(xml_child_node xcn, xn.children)
        {
            boost::apply_visitor(client::xml_child_node_printer(_tabsize+indent), xcn);
        }
        tab(indent);
        std::cout<< "}"<<std::endl;
    }
    
    ///////////////////////////////////////////////////////////////////////////
    //  Grammar for xml parsing
    ///////////////////////////////////////////////////////////////////////////
    
    template <typename Iterator>
    struct xml_grammar : qi::grammar<Iterator, xml_node(), qi::locals<std::string>, ascii::space_type >
    {
        xml_grammar():xml_grammar::base_type(xml_node_rule, "xmlnode")
        {
            tag_open = '<'
                >> !qi::lit('/')
                >> qi::lexeme[+(ascii::char_-'>')]
                >> '>';
            tag_close = "</"
                > ascii::string(qi::labels::_r1)
                > ascii::char_('>');
            text %= qi::lexeme[+(ascii::char_-'<')];
            xml_child_node_rule %= text | xml_node_rule;
            xml_node_rule %= tag_open[qi::labels::_a=qi::labels::_1]
                > *xml_child_node_rule
                > tag_close(qi::labels::_a);
            
            xml_node_rule.name("xmlnode");
            xml_child_node_rule.name("xmlchildnode");
            tag_open.name("openningtag");
            tag_close.name("closingtag");
            
            qi::on_error<qi::fail>
            (
                xml_node_rule,
                std::cout
                    << phoenix::val("ERROR: Expect ")
                    << qi::labels::_4
                    << phoenix::val(" at \"")
//                    << qi::labels::_3 << qi::labels::_2
                    << phoenix::val("\"")
                    << std::endl
            );
        
        }
        qi::rule<Iterator, xml_node(), qi::locals<std::string>, ascii::space_type> xml_node_rule;
        qi::rule<Iterator, std::string(), ascii::space_type> tag_open;
        qi::rule<Iterator, void(std::string), ascii::space_type> tag_close;
        qi::rule<Iterator, std::string(), ascii::space_type> text;
        qi::rule<Iterator, xml_child_node(), ascii::space_type> xml_child_node_rule;
    };
}

int main(int argc, char **argv)
{
    char const* filename;
    if (argc > 1)
    {
        filename = argv[1];
    }
    else
    {
        std::cerr << "Error: No input file provided." << std::endl;
        return 1;
    }
    
    std::ifstream in(filename, std::ios_base::in);
    
    if (!in)
    {
        std::cerr << "Error: Could not open input file: "
        << filename << std::endl;
        return 1;
    }

    std::string s; // = "<h>hello<i>sdr</i><j>werrwe</j></h>";
    
    in.unsetf(std::ios::skipws); // No white space skipping!
    std::copy(
              std::istream_iterator<char>(in),
              std::istream_iterator<char>(),
              std::back_inserter(s));
    
    std::string::const_iterator sbegin = s.begin();
    std::string::const_iterator send = s.end();
    client::xml_node ast;
    namespace qi=boost::spirit::qi;
    namespace ascii=boost::spirit::ascii;
    client::xml_grammar<std::string::const_iterator> xml_grammar;
    bool r = qi::phrase_parse(sbegin, send, xml_grammar, ascii::space, ast);
    std::cout << "parse success: " << r << std::endl;
    
    
    if (r && sbegin == send)
    {
        std::cout << "-------------------------\n";
        std::cout << "Parsing succeeded\n";
        std::cout << "-------------------------\n";
        client::xml_node_printer printer;
        printer(ast);
        return 0;
    }
    else
    {
        std::cout << "-------------------------\n";
        std::cout << "Parsing failed\n";
        std::cout << "-------------------------\n";
        return 1;
    }
}
