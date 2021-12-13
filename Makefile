# .+ 
#
# .context    : FIFOEE, FIFO of variable size data blocks over EEPROM
# .title      : main make file
# .kind       : make file
# .author     : Fabrizio Pollastri <mxgbot@gmail.com>
# .site       : Revello - Italy
# .creation   : 13-Dec-2021
# .copyright  : (c) 2021 Fabrizio Pollastri
# .license    : GNU Lesser General Public License version 3
# 
# .-

.PHONY: html clean


%.html: %.rst
	rst2html $< > $@

targets = README.html

html:	$(targets)
	$(MAKE) -C doc html

clean:
	$(MAKE) -C doc clean
	rm -f *.html

#### END ####
