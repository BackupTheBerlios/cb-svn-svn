test icons\svn.48.png -nt res\svn-icon.ico -o icons\svn.32.png -nt res\svn-icon.ico  &&  icons\png2ico res\svn-icon.ico icons\svn.48.png icons\svn.32.png || true
zip -u -j svn.zip res\svn.xrc icons\log.png || true
