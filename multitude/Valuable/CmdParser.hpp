/* Copyright (C) 2007-2013: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 * 
 */

#ifndef VALUABLE_CMDPARSER_HPP
#define VALUABLE_CMDPARSER_HPP

#include "Export.hpp"
#include <Radiant/StringUtils.hpp>

#include <QStringList>
#include <QSet>

namespace Valuable
{
  class Node;

  /// Command line parser.
  class VALUABLE_API CmdParser
  {
  public:
    /// Parses command line arguments to given Node object.
    /**
     * @param argc Number of arguments in argv
     * @param argv Array of arguments
     * @param opts The target object where the Attributes are stored
     *
     * If there is a Attribute named "foo" in opts, it can be set in command
     * line like this: <tt>--foo param</tt>
     * where param will be parsed with deserialize().
     *
     * If the Attribute name is only one letter, then the parser also recognizes
     * the command like argument that begins with one dash, like <tt>-o dir</tt>
     *
     * Boolean arguments (AttributeBool objects) don't use distinct parameter, but
     * they will be switched on by \c --name and off by \c --no-name.
     *
     * String lists are separated by semicolon (;) that can be escaped with
     * backslash: <tt>--css style1.css;style2.css</tt>
     *
     * Example:
     * \code
     * Node opts;
     * AttributeInt limit(&opts, "limit", 5);
     * AttributeString target(&opts, "target", "/tmp/target");
     * AttributeBool verbose(&opts, "v", false);
     * AttributeBool recursive(&opts, "recursive", true);
     * QStringList files = CmdParser::parse(argc, argv, opts);
     * \endcode
     * Example input: -v --limit 12 --no-recursive --target out file1.txt file2.txt
     *
     * @return List of arguments that didn't match any Attribute in opts.
     */
    static QStringList parse(int & argc, char * argv[],
                             Valuable::Node & opts);

    static QStringList parse(const QStringList & argv, Valuable::Node & opts);

    /// Parses command line arguments to given Node object.
    /**
      * @param argc Number of arguments in argv
      * @param argv Array of arguments
      * @param opts The target object where the Attributes are stored
      *
      * This is the non-static version of the parse function. This version
      * stores all parsed command line arguments in an internal set, which
      * can be queried by the \c is_parsed function.
      *
      * @return List of arguments that didn't match any Attribute in opts.
      */
    QStringList parseAndStore(int & argc, char * argv[],
                              Valuable::Node & opts);

    QStringList parseAndStore(const QStringList & argv, Valuable::Node & opts);

    /**
      * Query the CmdParser if a certain command line parameter has been parsed.
      *
      * @param name The name of the parameter to query
      *
      * Example:
      * \code
      * CmdParser parser;
      * parser.parseAndStore(argc, argv, opts);
      * if(parser.isParsed("foo") printf("--foo was parsed\n");
      * \endcode
      *
      * @return True if the parameter was parsed.
      */
    bool isParsed(const QString & name);

  private:
    QSet<QString> m_parsedArgs;
  };
}

#endif // VALUABLE_CMDPARSER_HPP
