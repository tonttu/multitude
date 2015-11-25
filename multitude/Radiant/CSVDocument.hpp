/* Copyright (C) 2007-2013: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 * 
 */

#ifndef RADIANT_CSVDOCUMENT_HPP
#define RADIANT_CSVDOCUMENT_HPP

#include "Export.hpp"

#include <QString>
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

    /// A single row of data
    typedef std::vector<QString> Row;
    /// A list of rows
    typedef std::list<Row> Rows;

    /// Constructor
    CSVDocument();
    /// Destructor
    ~CSVDocument();

    int loadFromString(const QString & csv, const char * delimiter = ",", bool removeQuotations = true);

    /** Load a file, and return the number of lines read. The file is assumed to be in the
        UTF-8 format.
        @param filename filename to read
        @param delimiter column delimiter
        @param removeQuotations Are the quotations around the value removed
        @return number of rows read */
    int load(const QString & filename, const char * delimiter = ",", bool removeQuotations = true);
    bool save(const QString & filename, const char * delimiter = ",", bool useQuotations = true);
    /** Finds a row in the document. For each row in the document,
        this function checks if the text in the cell at that column
        matches the argument key.

        @param key The key to match

        @param col The column that is used for matching

        @return If the key could not matched, return 0, otherwise returns a
        pointer to the row.
    */
    Row * findRow(const QString & key, unsigned col);

    /// Finds column on a given row
    /// @param key Valy of ccell to be searched
    /// @param rowIndex Row to search
    /// @return Number of column where the item was, -1 if not found.
    int findColumnOnRow(const QString & key, unsigned rowIndex);

    /// Returns an iterator to the first row in the document
    /// @return STL-like iterator to the beginning of the rows
    Rows::iterator begin() { return m_rows.begin(); }
    /// Returns an iterator to the first row in the document
    /// @return STL-like iterator to the beginning of the rows
    Rows::const_iterator begin() const { return m_rows.begin(); }
    /// Returns an iterator after the last row of the document
    /// @return STL-like iterator to the end of the rows
    Rows::iterator end() { return m_rows.end(); }
    /// Returns an iterator after the last row of the document
    /// @return STL-like iterator to the end of the rows
    Rows::const_iterator end() const { return m_rows.end(); }

    /// Returns the number of rows in the document
    /// @return number of rows
    unsigned rowCount() const { return (unsigned) m_rows.size(); }

    /** Returns a given row. If the index is out of range, zero pointer is returned.

      This method is quite slow, since it must iterate through each row to find the
      correct one. If performance is an issue you should use the iterator-functions instead.
      @param i index of the row
      @return the ith row
    */
    Row * row(unsigned i);
    /** Adds a new row to the end of the table.
        @return The newly created row.
    */
    Row * appendRow();

    /// Clears (empties) the entire CSVDocument
    void clear();

  private:


    Rows m_rows;
  };


}

#endif // CSVDOCUMENT_HPP
