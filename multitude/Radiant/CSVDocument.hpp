/* COPYRIGHT
 *
 * This file is part of Radiant.
 *
 * Copyright: MultiTouch Oy, Helsinki University of Technology and others.
 *
 * See file "Radiant.hpp" for authors and more details.
 *
 * This file is licensed under GNU Lesser General Public
 * License (LGPL), version 2.1. The LGPL conditions can be found in
 * file "LGPL.txt" that is distributed with this source package or obtained
 * from the GNU organization (www.gnu.org).
 *
 */

#ifndef RADIANT_CSVDOCUMENT_HPP
#define RADIANT_CSVDOCUMENT_HPP

#include <Radiant/Export.hpp>

#include <string>
#include <list>
#include <vector>

namespace Radiant {

  /** A simple class for loading CSV documents.

      CSV (Comma-Separeted Values) documents are basically spreadsheets, with information
      placed on rows, with an agreed separator between the cells. The Cells are expected to have
      quotation marks around the main content.
      The quotation marks are removed in the reading process, as are leading-, and trailing
      spaces.

  */


  class RADIANT_API CSVDocument
  {
  public:

    class RADIANT_API Row : public std::vector<std::wstring>
    {
    public:
      Row() {}
    };

    typedef std::list<Row> Rows;

    CSVDocument();
    ~CSVDocument();

    /** Load a file, and return the number of lines read. The file is assumed to be in the
        UTF-8 format.
    */
    int load(const char * filename, const char * delimiter = ",");
    /** Finds a row in the document. For each row in the document,
        this function checks if the text in the cell at that column
        matches the argument key.

        @param key The key to match

        @param col The column that is used for matching

        @return If the key could not matched, return 0, otherwise returns a
        pointer to the row.
    */
    Row * findRow(const std::wstring & key, unsigned col);

    /// Returns an iterator to the first row in the document
    Rows::iterator begin() { return m_rows.begin(); }
    Rows::const_iterator begin() const { return m_rows.begin(); }
    /// Returns an iterator after the last row of the document
    Rows::iterator end() { return m_rows.end(); }
    Rows::const_iterator end() const { return m_rows.end(); }

    /// Returns the number of rows in the document
    unsigned rowCount() const { return m_rows.size(); }

    /** Returns a given row. If the index is out of range, zero pointer is returned.

      This method is quite slow, since it must iterate through each row to find the
      correct one. If performance is an issue you should use the iterator-functions instead.
    */
    Row * row(unsigned index);

  private:


    Rows m_rows;
  };


}

#endif // CSVDOCUMENT_HPP
