;;; mpc-mode.el --- Makefile Project Creator mode for Emacs

;; Author: Jules Colding  <colding@42tools.com>
;; Maintainer: Jules Colding
;; Keywords: languages, faces, mpc

;; Copyright (C) 2008 Jules Colding <colding@42tools.com>
;;
;; This program is free software: you can redistribute it and/or modify
;; it under the terms of the GNU General Public License as published by
;; the Free Software Foundation, either version 3 of the License, or
;; (at your option) any later version.
;;
;; This program is distributed in the hope that it will be useful,
;; but WITHOUT ANY WARRANTY; without even the implied warranty of
;; MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
;; GNU General Public License for more details.
;;
;; You should have received a copy of the GNU General Public License
;; along with this program.  If not, see <http://www.gnu.org/licenses/>.

;; A major mode for editing MPC input. Please see:
;;
;;   http://downloads.ociweb.com/MPC/MakeProjectCreator.pdf
;;
;; Derived from autoconf-mode.el by Martin Buchholz (martin@xemacs.org)


;; Many thanks to the follwing kind people for extensions, bugfixes and
;; other contributions:
;;
;;   * William R. Otte <wotte@dre.vanderbilt.edu>
;;       Indentation and syntax table.
;; 

;; Put this file somewhere in your emacs load path and add the following
;; to your Emacs configuration file:
;;
;; (require 'mpc-mode)
;;
;;
;; You may also add something like this to the top of your MPC files
;; to force a specific indentation mode:
;;
;; // -*- Mode: MPC; tab-width: 2; indent-tabs-mode: t; -*-

;;; Code:

(require 'font-lock)

(defvar mpc-mode-hook nil)

(defvar mpc-mode-map
  (let ((mpc-mode-map (make-sparse-keymap)))
    (define-key mpc-mode-map '[(control c) (control c)] 'comment-region)
    (define-key mpc-mode-map '[(control j)]             'newline-and-indent)
    mpc-mode-map)
  "Keymap for MPC major mode")

(defvar mpc-font-lock-keywords
  `(
    ("\\(project\\)"              1 font-lock-warning-face t)
    ("\\(workspace\\)"            1 font-lock-warning-face t)
    ("(\\([^()]*\\))"             1 font-lock-constant-face t)
    ("\\(IDL_Files\\)"            1 font-lock-keyword-face t) 
    ("\\(Source_Files\\)"         1 font-lock-keyword-face t)
    ("\\(Header_Files\\)"         1 font-lock-keyword-face t)
    ("\\(Inline_Files\\)"         1 font-lock-keyword-face t)
    ("\\(Template_Files\\)"       1 font-lock-keyword-face t)
    ("\\(Resource_Files\\)"       1 font-lock-keyword-face t)
    ("\\(Pkgconfig_Files\\)"      1 font-lock-keyword-face t)
    ("\\(exclude\\)"              1 font-lock-type-face t)
    ("\\(custom_only\\)"          1 font-lock-type-face t)
    ("\\(cmdline\\)"              1 font-lock-type-face t)
    ("\\(avoids\\)"               1 font-lock-type-face t)
    ("\\(exename\\)"              1 font-lock-type-face t)
    ("\\(install =\\)"            1 font-lock-type-face t)
    ("\\(install +=\\)"           1 font-lock-type-face t)
    ("\\(install -=\\)"           1 font-lock-type-face t)
    ("\\(libs\\)"                 1 font-lock-type-face t)
    ("\\(lit_libs\\)"             1 font-lock-type-face t)
    ("\\(linkflags\\)"            1 font-lock-type-face t)
    ("\\(specific\\)"             1 font-lock-type-face t)
    ("\\(macros\\)"               1 font-lock-type-face t)
    ("\\(after\\)"                1 font-lock-type-face t)
    ("\\(libout\\)"               1 font-lock-type-face t)
    ("\\(libpaths\\)"             1 font-lock-type-face t)
    ("\\(includes\\)"             1 font-lock-type-face t)
    ("\\(automatic\\)"            1 font-lock-type-face t)
    ("\\(command\\)"              1 font-lock-type-face t)
    ("\\(output_option\\)"        1 font-lock-type-face t)
    ("\\(header_pre_extension\\)" 1 font-lock-type-face t)
    ("\\(header_outputext\\)"     1 font-lock-type-face t)
    ("\\(sharedname\\)"           1 font-lock-type-face t)
    ("\\(dynamicflags\\)"         1 font-lock-type-face t)
    ("\\(idlflags\\)"             1 font-lock-type-face t)
    ("\\(:\\)"                    1 font-lock-builtin-face t)
    ("\\( = \\)"                  1 font-lock-builtin-face t)
    ("\\(+=\\)"                   1 font-lock-builtin-face t)
    ("\\(-=\\)"                   1 font-lock-builtin-face t)
    ("\\(//\\)"                   1 font-lock-comment-face t)
    ("\\//\\(.*\\)"               1 font-lock-comment-face t)
    "default font-lock-keywords")
  )

;; Indenting logic
(defun mpc-indent-line ()
  "Indent current line as MPC directives"
  (interactive)
  (beginning-of-line)

  (if (bobp)
      (indent-line-to 0) ; if we are at start of file, zero indent
    (let ((not-found-hint t) cur-indent (close-brace nil))
      (save-excursion ; otherwise, we are not looking at a }, so we need to go back to find the
        (if (looking-at ".*}")
            (setq close-brace t))
        (while not-found-hint ; nearest indentation hint
          (forward-line -1)
          (if (looking-at ".*{")
              (progn
                (setq cur-indent (+ (current-indentation) tab-width))
                (setq not-found-hint nil))
            (if (looking-at ".*}")
                (progn
                  (setq cur-indent (current-indentation))
                  (if (< cur-indent 0)
                      (setq cur-indent 0))
                  (setq not-found-hint nil))
              (if (bobp)
                  (setq not-found-hint nil))))))
      (if close-brace
          (setq cur-indent (- cur-indent tab-width)))
      (if cur-indent
          (indent-line-to cur-indent)
        (indent-line-to 0))))
  )

;; Create a syntax table.  Derived from fundamental mode, it will automatically
;; highlight strings, and behave correctly on most words.
(defvar mpc-mode-syntax-table nil
  "syntax table used in mpc mode")
(setq mpc-mode-syntax-table (make-syntax-table))
(modify-syntax-entry ?_ "w" mpc-mode-syntax-table)     ; underscore is a valid part of a word
(modify-syntax-entry ?- "w" mpc-mode-syntax-table)     ; hyphen is a valid part of a word
(modify-syntax-entry ?/ ". 12b" mpc-mode-syntax-table) ; c++-style comments
(modify-syntax-entry ?\n "> b" mpc-mode-syntax-table)  ; c++-style comments

;;;###autoload
(defun mpc-mode ()
  "A major-mode to edit MPC files.
\\{mpc-mode-map}
"
  (interactive)
  (kill-all-local-variables)
  (use-local-map mpc-mode-map)

  (make-local-variable 'comment-start)
  (setq comment-start "//")
  (make-local-variable 'parse-sexp-ignore-comments)
  (setq parse-sexp-ignore-comments t)

  (make-local-variable 'tab-width)
  (make-local-variable 'font-lock-defaults)
  (make-local-variable 'indent-line-function)

  (setq major-mode 'mpc-mode)
  (setq mode-name "MPC")

  (setq font-lock-defaults `(mpc-font-lock-keywords nil t))
  (setq indent-line-function 'mpc-indent-line) 

  (set-syntax-table mpc-mode-syntax-table)
  (run-hooks 'mpc-mode-hook)
  )

(add-to-list 'auto-mode-alist '("\\.mwb\\'" . mpc-mode))
(add-to-list 'auto-mode-alist '("\\.mwc\\'" . mpc-mode))
(add-to-list 'auto-mode-alist '("\\.mpb\\'" . mpc-mode))
(add-to-list 'auto-mode-alist '("\\.mpc\\'" . mpc-mode))


(provide 'mpc-mode)

;;; mpc-mode.el ends here
