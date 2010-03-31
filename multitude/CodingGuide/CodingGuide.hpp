/* COPYRIGHT

    All C/C++ files begin with a copyright notice. Whenever you create
    a new file, you should copy the copyright from another file in the
    same directory.

    The first line of the notice
 looks like the line above. This is
    because then we can replace them atomatically. 

    NOTE: Anything written into this space may get lost during the
    automatic update.

    Basic license mumbo-jumbo is present in this secion.

 */

/*
   Extra copyright:

   By making non-standard copyright we can guarantee that it will not
   be overwritten by notice updates.
*/

#ifndef CODINGGUIDE_CODINGGUIDE_HP
#define CODINGGUIDE_CODINGGUIDE_HP

/// Coding example, for consistent programming style
/** <B> Some general comments</B>

    A library or application of name Xyz has a header Xyz.hpp that
    contains overall documentation of the project.
    
    This way there is standard place for this documentation and it
    always moves with the library. The documentation is above a
    namespace-block so that Doxygen can connect this documentation to
    the projet.

    As you see all public documentation is made to work with Doxygen.

    The namespace of the project is the same as the project/directory
    name. The resulting library/application also carries exactly the
    same name (including capitalization).

    We avoid static/global variables as much as possible.
    
    There are at most 80 characters per line. This way you can fit 2
    (or even 3) code-editors side by side :-)

    We make all classes as thread-safe as possible. The classes do not
    need to be more thread-safe than possible.

    The coding conventions have not been around from the beginning. As
    a result there are a lot of files with slightly varying
    styles. There is little point in trying to fix all them right now
    to match this guide. Instead, all new code should follow this
    guide. Old code can be re-formatted as we go.

    This is also good place for general author list for the project:

    @author Tommi Ilmonen
*/

namespace CodingGuide {

  /** There is no need to have any real definitions here, this file is
      only for documentation. For the sake of the example, we will put
      project-specific global functions here. */

  void initCodingGuide();

  /// Global variables are prefixed with g_
  int g_globalVariable;  
}


#endif
