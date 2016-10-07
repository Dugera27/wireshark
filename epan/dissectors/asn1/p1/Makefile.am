# Wireshark - Network traffic analyzer
# By Gerald Combs <gerald@wireshark.org>
# Copyright 1998 Gerald Combs
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.

include ../Makefile.inc

PROTOCOL_NAME=p1

EXPORT_FILES = $(PROTOCOL_NAME)-exp.cnf

EXT_ASN_FILE_LIST =

ASN_FILE_LIST = \
	MTAAbstractService.asn \
	MTSAbstractService.asn \
	MTSAccessProtocol.asn \
	MHSProtocolObjectIdentifiers.asn \
	MTSUpperBounds.asn

# The packet-$(PROTOCOL_NAME)-template.h and $(PROTOCOL_NAME).asn
# files do not exist for all protocols: Please add/remove as required.
EXTRA_DIST = \
	$(EXTRA_DIST_COMMON) \
	$(ASN_FILE_LIST) \
	packet-$(PROTOCOL_NAME)-template.c \
	packet-$(PROTOCOL_NAME)-template.h \
	$(PROTOCOL_NAME).cnf

SRC_FILES = \
	$(EXTRA_DIST) \
	$(EXT_ASN_FILE_LIST)

A2W_FLAGS = -b -C

EXTRA_CNF = \
	$(builddir)/../x509af/x509af-exp.cnf \
	$(builddir)/../x509ce/x509ce-exp.cnf \
	$(builddir)/../x509if/x509if-exp.cnf \
	$(builddir)/../x509sat/x509sat-exp.cnf \
	$(builddir)/../ros/ros-exp.cnf \
	$(builddir)/../rtse/rtse-exp.cnf

EXPORT_DEPENDS = \
	$(builddir)/../x509af/x509af-exp.cnf

$(builddir)/../x509af/x509af-exp.cnf:
	(cd $(builddir)/../x509af && $(MAKE_CNF_EXPORT))

$(builddir)/../x509ce/x509ce-exp.cnf:
	(cd $(builddir)/../x509ce && $(MAKE_CNF_EXPORT))

$(builddir)/../x509if/x509if-exp.cnf:
	(cd $(builddir)/../x509if && $(MAKE_CNF_EXPORT))

$(builddir)/../x509sat/x509sat-exp.cnf:
	(cd $(builddir)/../x509sat && $(MAKE_CNF_EXPORT))

$(builddir)/../ros/ros-exp.cnf:
	(cd $(builddir)/../ros && $(MAKE_CNF_EXPORT))

$(builddir)/../rtse/rtse-exp.cnf:
	(cd $(builddir)/../rtse && $(MAKE_CNF_EXPORT))