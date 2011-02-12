/* -*- c++ -*- */

//=============================================================================
/**
 *  @file    HTTP_Config.h
 *
 *  $Id: HTTP_Config.h 80826 2008-03-04 14:51:23Z wotte $
 *
 *  @author James Hu
 */
//=============================================================================


// = Forward declaration.
class HTTP_Config_Info;

/**
 * @class HTTP_Config
 *
 * @brief Stores server configuration information.
 * Someday, this will be hip and cool and be able to parse
 * NCSA httpd style config files like Apache does.  For now,
 * I'm just going to hack in environment variable stuff.
 * Designed around Singleton pattern.
 */
class HTTP_Config
{
public:
  /// Access the Singleton.
  static HTTP_Config_Info *instance (void);

private:
  /// Store the Singleton.
  static HTTP_Config_Info *instance_;
};

/**
 * @class HTTP_Config_Info
 *
 * @brief This is where the information is really stored.
 */
class HTTP_Config_Info
{
friend class HTTP_Config;
public:
  HTTP_Config_Info (void);
  ~HTTP_Config_Info (void);

  // Accessors to the information

  /// Where the root of the document tree begins.  This prevents
  /// clients from being able to examine your entire filesystem.
  const char *document_root (void) const;

  /// A search path for CGI files.
  const char *cgi_path (void) const;

  /// The directory which is appended to a home user directory, e.g.,
  /// ".www-docs" or "public_html".
  const char *user_dir (void) const;

  /// What is the default index file for a directory, e.g.,
  /// "index.html".
  const char *dir_index (void) const;

  /// Will the server support proxy requests?
  int proxy_flag (void) const;

private:
  // = Accesors that can set the data

  const char *document_root (const char *dr_string);
  const char *cgi_path (const char *cp_string);
  const char *user_dir (const char *ud_string);
  const char *dir_index (const char *di_string);

  int proxy_flag (int pf);

private:
  // = Data members

  /// The directory root from which documents will be fetched
  const char *document_root_;

  /// The directories from which to expect CGI scripts
  const char *cgi_path_;

  /// Name of the sub-directory where user Web pages are
  const char *user_dir_;

  /// Name of the Web page to present in place of a directory listing
  const char *dir_index_;

  /// Should we support proxy requests?  Ignored for now.
  int proxy_flag_;
};
