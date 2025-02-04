(defun highlight-log-file ()
  (interactive)
  (highlight-lines-matching-regexp "00000000" 'hi-pink)
  (highlight-lines-matching-regexp "00000001" 'hi-red)
  (highlight-lines-matching-regexp "6b8b4567" 'hi-blue)
  (highlight-lines-matching-regexp "e193b625" 'hi-blue)
  (highlight-lines-matching-regexp "a83fbe2c" 'hi-blue)
  (highlight-lines-matching-regexp "6b8b4567" 'hi-blue)
  (highlight-lines-matching-regexp "00384650" 'hi-blue)
  (highlight-regexp "6b8b4567" 'hi-red-b)
  (highlight-regexp "e193b625" 'hi-red-b)
  (highlight-regexp "a83fbe2c" 'hi-red-b)
  (highlight-regexp "6b8b4567" 'hi-red-b)
  (highlight-regexp "00384650" 'hi-red-b))

; (global-set-key (kbd "C-c h") 'highlight-log-file)

