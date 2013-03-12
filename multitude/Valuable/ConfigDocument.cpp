/* Copyright (C) 2007-2013: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 * 
 */


#include "ConfigDocument.hpp"

#include <Radiant/Trace.hpp>

#include <fstream>
#include <istream>

/// @todo Change everything in here to use QString
/// @todo make everything const-correct

namespace Valuable
{  

ConfigDocument::ConfigDocument(void)
{
}

ConfigDocument::~ConfigDocument(void)
{
}

bool ConfigDocument::readConfigFile(const char *fileName)
{
  std::string str;

  std::ifstream input(fileName, std::ios_base::in);

  if(!input) {
    Radiant::error("ConfigDocument::readConfigFile # failed to open '%s' for reading", fileName);
    return false;
  }

  int depth = 0;
  //bool flag = false, atFlag = false;

  int k = 0;

  while(std::getline(input,str)) {
    trimSpaces(str);
    if(str == "")
      continue;

    if(parseLine(str)==ELEMENT_START) {
      std::string s;
      depth++;
      ConfigElement elm,e;
      size_t ps=str.find(",");
      if(ps<10000) {

        e.m_elementName=str.substr(0,ps).c_str();
        size_t ps2=str.find("{");
        e.m_type=str.substr(ps+1,ps2-ps-1).c_str();

      }
      else {
        e.m_elementName=str.substr(0,str.find("{")-1).c_str();
      }

      trimSpaces(e.m_elementName);

      // Radiant::info("E1 : %s", e.m_elementName.c_str());

      e.m_depth=depth;
      elm.m_nodes.push_back(e);
      k++;

      while(std::getline(input,s)) {
        trimSpaces(s);
        if(s!="") {
          if(parseLine(s)==ELEMENT_START) {

            depth++;
            ConfigElement child=ConfigElement();
            size_t ps=s.find(",");
            if(ps<10000) {

              child.m_elementName=s.substr(0,ps).c_str();
              child.m_type=s.substr(ps+1,s.find("{")-ps-1).c_str();

            }
            else {
              child.m_elementName=s.substr(0,s.find("{")-1).c_str();
            }

            trimSpaces(child.m_elementName);

            // Radiant::info("E2: %s", child.m_elementName.c_str());

            child.m_depth=depth;
            elm.m_nodes.push_back(child);
            k++;

          }
          else if(parseLine(s)==ATTRIBUTE) {
            ConfigValue att=ConfigValue();
            for(int i = 0; i < (int) s.length();i++)
              if(s[i]=='\"')
                s[i]=' ';
            size_t pos=s.find("=");

            att.m_key=s.substr(0,pos).c_str();
            trimSpaces(att.m_key);
            att.m_value=s.substr(pos+1,s.length()).c_str();
            trimSpaces(att.m_value);

            att.m_depth=depth;
            elm.m_nodes[k-1].m_values.push_back(att);

          }
          else if(parseLine(s)==ELEMENT_END) {
            depth--;

            if(depth==0) {
              k=0;

              for(int i= (int) elm.m_nodes.size()-1;i>0;i--) {
                if(elm.m_nodes[i].m_depth>elm.m_nodes[i-1].m_depth) {
                  elm.m_nodes[i-1].m_nodes.push_back(elm.m_nodes[i]);
                  // Radiant::info("E3 : %s", elm.m_nodes[i].m_elementName.c_str());
                }
                else {

                  for(int j=i-2;j>=0;j--) {
                    if(elm.m_nodes[j].m_depth<elm.m_nodes[i].m_depth &&
                       (elm.m_nodes[i].m_depth-elm.m_nodes[j].m_depth)==1 ) {
                      elm.m_nodes[j].m_nodes.push_back(elm.m_nodes[i]);
                      // Radiant::info("E4 : %s", elm.m_nodes[i].m_elementName.c_str());
                      break;
                    }
                  }
                }
              }

              ConfigElement el = ConfigElement();
              el.m_nodes.push_back(elm.m_nodes[0]);

              // Radiant::info("E5 : %s", elm.m_nodes[0].m_elementName.c_str());
              m_doc.m_nodes.push_back(el);
              elm.m_nodes.clear();
              elm.m_values.clear();
              //flag=false;
              //atFlag=false;
            }
          }
        }
      }
    }
  }
  input.close();

  for(unsigned i = 0; i < m_doc.childCount(); i++) {
    ConfigElement & c1 = m_doc.child(i);
    if(c1.childCount() == 1 && c1.elementName().isEmpty()) {
      c1 = ConfigElement(c1.child(0));
    }
  }
  return true;
}



void ConfigDocument::writeConfigFile(const char *fileName)
{
  std::ofstream output(fileName);
  writeConfig(output);
  output.close();
}

void ConfigDocument::writeConfig(std::ostream & output)
{
  std::string aa = getConfigText(m_doc).toStdString();

  //	for(int i=ss.size()-1;i>=0;i--)
  //		output<<ss[i];

  output << aa;
}

void ConfigDocument::trimSpaces( std::string & str)
{
  size_t startpos = str.find_first_not_of(" \t\r");
  size_t endpos = str.find_last_not_of(" \t\r");

  if(( std::string::npos == startpos ) || ( std::string::npos == endpos)) {
    str = "";
  }
  else
    str = str.substr( startpos, endpos-startpos+1 );

}
void ConfigDocument::trimSpaces( QString & str)
{
  std::string tmp = str.toStdString();
  trimSpaces(tmp);
  str = tmp.c_str();
}
ConfigElement *ConfigDocument::getConfigElement(const QString & elementName)
{
  bool found=false;

  ConfigElement *f=findConfigElement(m_doc,elementName,found);
  if(f)
    return f;
  else
    return 0;

}
ConfigElement *ConfigDocument::getConfigElement(const QString & key,
                                                const QString & value)
{
  bool found=false;

  ConfigElement *f=findConfigElement(m_doc,found,key,value);
  if(f)
    return f;
  else
    return 0;

}

ConfigElement *ConfigDocument::findConfigElement
(ConfigElement &e,bool &found,const QString & key,const QString & value)
{
  for(int i=0;i < (int)e.m_nodes.size() ;i++) {
    ConfigElement *w;
    w=findConfigElement(e.m_nodes[i],found,key,value);
    if(found)
      return w;
  }

  for(int j=0;j<(int)e.m_values.size();j++) {
    std::string ke=e.m_values[j].m_key.toStdString();
    trimSpaces(ke);
    std::string val=e.m_values[j].m_value.toStdString();
    trimSpaces(val);

    if(key==ke.c_str() && value==val.c_str()) {
      found=true;

      return &e;

    }
  }

  return 0;
}

ConfigElement *ConfigDocument::findConfigElement
(ConfigElement &e,const QString & elementName,bool &found)
{
  for(int i=0;i < (int)e.m_nodes.size() ;i++) {
    ConfigElement *w;
    w=findConfigElement(e.m_nodes[i],elementName,found);
    if(found)
      return w;

  }

  std::string s=e.m_elementName.toStdString();
  trimSpaces(s);

  if(s.c_str()==elementName) {
    found=true;

    return &e;

  }

  return 0;
}

static std::string __indent(int recursion)
{
  //return QString(recursion*2, QChar::fromLatin1(' '));
  std::string res;

  for(int i = 0; i < recursion; i++)
    res += "  ";

  return res;
}

QString ConfigDocument::getConfigText(ConfigElement e, int recursion)
{
  std::string str;
  std::string ind(__indent(recursion));
  std::string ind2(ind + "  ");

  /* printf("Element name \"%s\" (%d)\n", e.m_elementName.c_str(),
     recursion);*/

  {
    if(e.m_elementName != "") {

      str += ind;

      if(e.m_type=="")
        str+=e.m_elementName.toStdString()+" {\n";
      else
        str+=(e.m_elementName+","+e.m_type+" {\n").toStdString();
    }
    else {
    }

    for(int j=0;j < (int) e.m_values.size();j++) {
      trimSpaces(e.m_values[j].m_value);
      str += (ind2.c_str() + e.m_values[j].m_key+"="+"\""+e.m_values[j].m_value+"\""+"\n").toStdString();

    }
    for(int i = 0; i < (int) e.m_nodes.size(); i++)
      str += getConfigText(e.m_nodes[i], recursion + 1).toStdString();

    if(e.m_elementName != "") {
      str += ind + "}\n";
    }
  }
  return str.c_str();
}

ConfigDocument::ParseFlags ConfigDocument::parseLine(std::string line)
{
  if(line[line.length()-1]=='{')
    return ELEMENT_START;
  else if(line[line.length()-1]=='}')
    return ELEMENT_END;
  else {
    size_t pos=line.find("=");
    if(pos == std::string::npos)
      return NOT_VALID;
    else
      return ATTRIBUTE;
  }
}

bool ConfigDocument::getline(FILE * input, std::string & str)
{
  str.clear();
  bool ok = true;
  int n = 0;
  while(ok) {
    char tmp = 0;
    int got = (int) fread(& tmp, 1, 1, input);
    if(got) {
      if(tmp != '\n')
        str += tmp;
      else if(n)
        return true;
      n++;
    }
    else
      ok = false;
  }

  return n != 0;
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

void convert(DOMDocument & doc, DOMElement & to, const ConfigElement & from)
{
  for(unsigned i = 0; i < from.childCount(); i++) {
    DOMElement child(doc.createElement(from.elementName()));
    convert(doc, child, from.child(i));
    to.appendChild(child);
  }

  for(unsigned i = 0; i < from.valueCount(); i++) {
    const ConfigValue & v = from.value(i);
    DOMElement child(doc.createElement(v.key()));

    child.setTextContent(v.value());

    to.appendChild(child);
  }
}

void convert(ConfigElement & to, DOMElement from)
{
  to.clear();

  to.setType(from.getAttribute("type"));
  to.setElementName(from.getTagName());

  DOMElement::NodeList nodes = from.getChildNodes();

  for(DOMElement::NodeList::iterator it = nodes.begin(); it != nodes.end(); ++it) {

    DOMElement de = (*it);

    DOMElement::NodeList nodes2 = de.getChildNodes();

    if(!nodes2.empty()) {
      ConfigElement tmp;
      convert(tmp, de);
      to.addElement(tmp);
    }
    else {
      ConfigValue tmp(de.getTagName(), de.getTextContent());
      to.addValue(tmp);
    }
  }

}
}
