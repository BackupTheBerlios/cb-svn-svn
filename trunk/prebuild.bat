test icons\svn.48.png -nt res\svn-icon.ico -o icons\svn.32.png -nt res\svn-icon.ico  &&  icons\png2ico res\svn-icon.ico icons\svn.48.png icons\svn.32.png || true
test res\svn.xrc -nt svn.zip -o icons\log.png -nt svn.zip  &&  zip -j svn.zip res\svn.xrc icons\log.png || true
