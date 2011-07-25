/* COPYRIGHT
 *
 * This file is part of Valuable.
 *
 * Copyright: MultiTouch Oy, Helsinki University of Technology and others.
 *
 * See file "Valuable.hpp" for authors and more details.
 *
 * This file is licensed under GNU Lesser General Public
 * License (LGPL), version 2.1. The LGPL conditions can be found in 
 * file "LGPL.txt" that is distributed with this source package or obtained 
 * from the GNU organization (www.gnu.org).
 * 
 */

#ifndef VALUABLE_CMDPARSER_HPP
#define VALUABLE_CMDPARSER_HPP

#include "Export.hpp"
#include <Radiant/StringUtils.hpp>

#include <set>
#include <string>

namespace Valuable
{
  class HasValues;

  /// Command line parser.
  class VALUABLE_API CmdParser
  {
  public:
    /// Parses command line arguments to given HasValues object.
    /**
     * If there is a ValueObject named "foo" in opts, it can be set in command
     * line like this: <tt>--foo param</tt>
     * where param will be parsed with deserialize().
     *
     * If the ValueObject name is only one letter, then the parser also recognizes
     * the command like argument that begins with one dash, like <tt>-o dir</tt>
     *
     * Boolean arguments (ValueBool objects) don't use distinct parameter, but
     * they will be switched on by \c --name and off by \c --no-name.
     *
     * Example:
     * \code
     * HasValues opts;
     * ValueInt limit(&opts, "limit", 5);
     * ValueString target(&opts, "target", "/tmp/target");
     * ValueBool verbose(&opts, "v", false);
     * ValueBool recursive(&opts, "recursive", true);
     * StringList files = CmdParser::parse(argc, argv, opts);
     * \endcode
     * Example input: -v --limit 12 --no-recursive --target out file1.txt file2.txt
     *
     * @return List of arguments that didn't match any ValueObject in opts.
     */
    static Radiant::StringUtils::StringList parse(int argc, char * argv[],
                                                  Valuable::HasValues & opts);

    /// Parses command line arguments to given HasValues object.
    /**
      * This is the non-static version of the parse function. This version
      * stores all parsed command line arguments in an internal set, which
      * can be queried by the \c is_parsed function.
      *
      * @return List of arguments that didn't match any ValueObject in opts.
      */
    Radiant::StringUtils::StringList parse_and_store(int argc, char * argv[],
                                            Valuable::HasValues & opts);

    /**
      * Query the CmdParser if a certain command line parameter has been parsed.
      *
      * Example:
      * \code
      * CmdParser parser;
      * parser.parse_args(argc, argv, opts);
      * if(parser.is_parsed("foo") printf("--foo was parsed\n");
      * \endcode
      */
    bool is_parsed(std::string name);

  private:
    std::set<std::string> m_parsedArgs;
  };
}

#endif // VALUABLE_CMDPARSER_HPP
