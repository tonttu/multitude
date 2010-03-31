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

#include <Radiant/Export.hpp>
#include <Radiant/StringUtils.hpp>

namespace Valuable
{
  class HasValues;

  /// Command line parser.
  class RADIANT_API CmdParser
  {
  public:
    /// Parses command line arguments to given HasValues object.
    /**
     * If there is a ValueObject named "foo" in opts, it can be set in command
     * line like this: <tt>--foo param</tt>
     * where param will be parsed with deserializeXML().
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
    static Radiant::StringUtils::StringList parse(int argc, char * argv[], Valuable::HasValues & opts);
  };
}

#endif // VALUABLE_CMDPARSER_HPP
