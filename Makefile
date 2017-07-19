# --------------------------------------------------------------------------
#  This program is free software; you can redistribute it and/or modify
#  it under the terms of the GNU General Public License version 2 as
#  published by the Free Software Foundation.
#  
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#  
#  You should have received a copy of the GNU General Public License
#  along with this program; if not, write to the Free Software
#  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
#  
#  As a special exception, you may use this file as part of a free software
#  library without restriction.  Specifically, if other files instantiate
#  templates or use macros or inline functions from this file, or you compile
#  this file and link it with other files to produce an executable, this
#  file does not by itself cause the resulting executable to be covered by
#  the GNU General Public License.  This exception does not however
#  invalidate any other reasons why the executable file might be covered by
#  the GNU General Public License.
# ---------------------------------------------------------------------------

# Author: Gabriele Mencagli <mencagli@di.unipi.it>
# Date:   March 2016

FF_ROOT		= $(HOME)/fastflow
FUZZY_ROOT	= $(HOME)/fuzzylite/fuzzylite
MAMMUT_ROOT = $(HOME)/mammut

CXX 		= g++
CXXFLAGS	= -std=c++11
INCLUDES	= -I $(FF_ROOT) -I $(FUZZY_ROOT) -I $(PWD)/include
LIBPATH		= -L $(FUZZY_ROOT)/release/bin
MACROS 		= -DNDEBUG -DTRACE_FASTFLOW -DNO_DEFAULT_MAPPING -DFL_CPP11=ON -DDIM=8 -DLOG
OPTFLAGS	= -g -O3 -finline-functions
LDFLAGS		= -pthread -lfuzzylite

ifeq ($(shell hostname -s), pianosa)
	MACROS 	 += -DENERGY -DPIANOSA
	INCLUDES += -I $(MAMMUT_ROOT)
	LIBPATH	 += -L $(MAMMUT_ROOT)/mammut
	LDFLAGS	 += -lmammut
else ifeq ($(shell hostname -s), pianosau)
	MACROS 	 += -DENERGY -DPIANOSAU
	INCLUDES += -I $(MAMMUT_ROOT)
	LIBPATH	 += -L $(MAMMUT_ROOT)/mammut
	LDFLAGS	 += -lmammut
else ifeq ($(shell hostname -s), repara)
	MACROS 	 += -DENERGY -DREPARA
	INCLUDES += -I $(MAMMUT_ROOT)
	LIBPATH	 += -L $(MAMMUT_ROOT)/mammut
	LDFLAGS	 += -lmammut
else ifeq ($(shell hostname -s), rephrase)
	MACROS	 += -DREPHRASE
else ifeq ($(shell hostname -s), ninja)
	MACROS 	 += -DENERGY -DNINJA
	INCLUDES += -I $(MAMMUT_ROOT)
	LIBPATH	 += -L $(MAMMUT_ROOT)/mammut
	LDFLAGS	 += -lmammut
endif

TARGETS_GEN = generator
TARGETS_PLQ = plq_skyline_pb plq_skyline_pb_static plq_skyline_pb_iop plq_skyline_pb_iop_static plq_skyline_tb plq_skyline_tb_static plq_skyline_tb_iop plq_skyline_tb_iop_static plq_skyline_as plq_skyline_as_static plq_skyline_as_iop plq_skyline_as_iop_static plq_skyline_aspid plq_skyline_aspid_static plq_skyline_aspid_iop plq_skyline_aspid_iop_static plq_topd_pb plq_topd_pb_static plq_topd_pb_iop plq_topd_pb_iop_static plq_topd_tb plq_topd_tb_static plq_topd_tb_iop plq_topd_tb_iop_static plq_topd_as plq_topd_as_static plq_topd_as_iop plq_topd_as_iop_static plq_topd_aspid plq_topd_aspid_static plq_topd_aspid_iop plq_topd_aspid_iop_static
TARGETS_PF = pf_skyline_pb pf_skyline_pb_static pf_skyline_pb_iop pf_skyline_pb_iop_static pf_skyline_tb pf_skyline_tb_static pf_skyline_tb_iop pf_skyline_tb_iop_static pf_skyline_as pf_skyline_as_static pf_skyline_as_iop pf_skyline_as_iop_static pf_skyline_aspid pf_skyline_aspid_static pf_skyline_aspid_iop pf_skyline_aspid_iop_static pf_topd_pb pf_topd_pb_static pf_topd_pb_iop pf_topd_pb_iop_static pf_topd_tb pf_topd_tb_static pf_topd_tb_iop pf_topd_tb_iop_static pf_topd_as pf_topd_as_static pf_topd_as_iop pf_topd_as_iop_static pf_topd_aspid pf_topd_aspid_static pf_topd_aspid_iop pf_topd_aspid_iop_static

.DEFAULT_GOAL := all
.PHONY: all gen plq pf clean cleanall
.SUFFIXES: .cpp

generator: src/generator.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) $(MACROS) $(OPTFLAGS) -o bin/generator src/generator.cpp -pthread

plq_skyline_pb: src/pf_query.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) $(LIBPATH) $(MACROS) -DSKYLINE -DPLQ_ONLY -DPB_RR_SCHED $(OPTFLAGS) -o bin/plq_skyline_pb src/pf_query.cpp $(LDFLAGS)

plq_skyline_pb_static: src/pf_query.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) $(LIBPATH) $(MACROS) -DSKYLINE -DPLQ_ONLY -DPB_RR_SCHED -DNO_ELASTIC $(OPTFLAGS) -o bin/plq_skyline_pb_static src/pf_query.cpp $(LDFLAGS)

plq_skyline_pb_iop: src/pf_query.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) $(LIBPATH) $(MACROS) -DSKYLINE -DPLQ_ONLY -DIOP -DPB_RR_SCHED $(OPTFLAGS) -o bin/plq_skyline_pb_iop src/pf_query.cpp $(LDFLAGS)

plq_skyline_pb_iop_static: src/pf_query.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) $(LIBPATH) $(MACROS) -DSKYLINE -DPLQ_ONLY -DIOP -DPB_RR_SCHED -DNO_ELASTIC $(OPTFLAGS) -o bin/plq_skyline_pb_iop_static src/pf_query.cpp $(LDFLAGS)

plq_skyline_tb: src/pf_query.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) $(LIBPATH) $(MACROS) -DSKYLINE -DPLQ_ONLY -DTB_RR_SCHED $(OPTFLAGS) -o bin/plq_skyline_tb src/pf_query.cpp $(LDFLAGS)

plq_skyline_tb_static: src/pf_query.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) $(LIBPATH) $(MACROS) -DSKYLINE -DPLQ_ONLY -DTB_RR_SCHED -DNO_ELASTIC $(OPTFLAGS) -o bin/plq_skyline_tb_static src/pf_query.cpp $(LDFLAGS)

plq_skyline_tb_iop: src/pf_query.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) $(LIBPATH) $(MACROS) -DSKYLINE -DPLQ_ONLY -DIOP -DTB_RR_SCHED $(OPTFLAGS) -o bin/plq_skyline_tb_iop src/pf_query.cpp $(LDFLAGS)

plq_skyline_tb_iop_static: src/pf_query.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) $(LIBPATH) $(MACROS) -DSKYLINE -DPLQ_ONLY -DIOP -DTB_RR_SCHED -DNO_ELASTIC $(OPTFLAGS) -o bin/plq_skyline_tb_iop_static src/pf_query.cpp $(LDFLAGS)

plq_skyline_as: src/pf_query.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) $(LIBPATH) $(MACROS) -DSKYLINE -DPLQ_ONLY -DADAPTIVE_SCHED $(OPTFLAGS) -o bin/plq_skyline_as src/pf_query.cpp $(LDFLAGS)

plq_skyline_as_static: src/pf_query.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) $(LIBPATH) $(MACROS) -DSKYLINE -DPLQ_ONLY -DADAPTIVE_SCHED -DNO_ELASTIC $(OPTFLAGS) -o bin/plq_skyline_as_static src/pf_query.cpp $(LDFLAGS)

plq_skyline_as_iop: src/pf_query.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) $(LIBPATH) $(MACROS) -DSKYLINE -DPLQ_ONLY -DIOP -DADAPTIVE_SCHED $(OPTFLAGS) -o bin/plq_skyline_as_iop src/pf_query.cpp $(LDFLAGS)

plq_skyline_as_iop_static: src/pf_query.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) $(LIBPATH) $(MACROS) -DSKYLINE -DPLQ_ONLY -DIOP -DADAPTIVE_SCHED -DNO_ELASTIC $(OPTFLAGS) -o bin/plq_skyline_as_iop_static src/pf_query.cpp $(LDFLAGS)

plq_skyline_aspid: src/pf_query.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) $(LIBPATH) $(MACROS) -DSKYLINE -DPLQ_ONLY -DADAPTIVE_SCHED_PID $(OPTFLAGS) -o bin/plq_skyline_aspid src/pf_query.cpp $(LDFLAGS)

plq_skyline_aspid_static: src/pf_query.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) $(LIBPATH) $(MACROS) -DSKYLINE -DPLQ_ONLY -DADAPTIVE_SCHED_PID -DNO_ELASTIC $(OPTFLAGS) -o bin/plq_skyline_aspid_static src/pf_query.cpp $(LDFLAGS)

plq_skyline_aspid_iop: src/pf_query.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) $(LIBPATH) $(MACROS) -DSKYLINE -DPLQ_ONLY -DIOP -DADAPTIVE_SCHED_PID $(OPTFLAGS) -o bin/plq_skyline_aspid_iop src/pf_query.cpp $(LDFLAGS)

plq_skyline_aspid_iop_static: src/pf_query.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) $(LIBPATH) $(MACROS) -DSKYLINE -DPLQ_ONLY -DIOP -DADAPTIVE_SCHED_PID -DNO_ELASTIC $(OPTFLAGS) -o bin/plq_skyline_aspid_iop_static src/pf_query.cpp $(LDFLAGS)

plq_topd_pb: src/pf_query.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) $(LIBPATH) $(MACROS) -DTOPD -DPLQ_ONLY -DPB_RR_SCHED $(OPTFLAGS) -o bin/plq_topd_pb src/pf_query.cpp $(LDFLAGS)

plq_topd_pb_static: src/pf_query.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) $(LIBPATH) $(MACROS) -DTOPD -DPLQ_ONLY -DPB_RR_SCHED -DNO_ELASTIC $(OPTFLAGS) -o bin/plq_topd_pb_static src/pf_query.cpp $(LDFLAGS)

plq_topd_pb_iop: src/pf_query.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) $(LIBPATH) $(MACROS) -DTOPD -DPLQ_ONLY -DIOP -DPB_RR_SCHED $(OPTFLAGS) -o bin/plq_topd_pb_iop src/pf_query.cpp $(LDFLAGS)

plq_topd_pb_iop_static: src/pf_query.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) $(LIBPATH) $(MACROS) -DTOPD -DPLQ_ONLY -DIOP -DPB_RR_SCHED -DNO_ELASTIC $(OPTFLAGS) -o bin/plq_topd_pb_iop_static src/pf_query.cpp $(LDFLAGS)

plq_topd_tb: src/pf_query.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) $(LIBPATH) $(MACROS) -DTOPD -DPLQ_ONLY -DTB_RR_SCHED $(OPTFLAGS) -o bin/plq_topd_tb src/pf_query.cpp $(LDFLAGS)

plq_topd_tb_static: src/pf_query.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) $(LIBPATH) $(MACROS) -DTOPD -DPLQ_ONLY -DTB_RR_SCHED -DNO_ELASTIC $(OPTFLAGS) -o bin/plq_topd_tb_static src/pf_query.cpp $(LDFLAGS)

plq_topd_tb_iop: src/pf_query.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) $(LIBPATH) $(MACROS) -DTOPD -DPLQ_ONLY -DIOP -DTB_RR_SCHED $(OPTFLAGS) -o bin/plq_topd_tb_iop src/pf_query.cpp $(LDFLAGS)

plq_topd_tb_iop_static: src/pf_query.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) $(LIBPATH) $(MACROS) -DTOPD -DPLQ_ONLY -DIOP -DTB_RR_SCHED -DNO_ELASTIC $(OPTFLAGS) -o bin/plq_topd_tb_iop_static src/pf_query.cpp $(LDFLAGS)

plq_topd_as: src/pf_query.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) $(LIBPATH) $(MACROS) -DTOPD -DPLQ_ONLY -DADAPTIVE_SCHED $(OPTFLAGS) -o bin/plq_topd_as src/pf_query.cpp $(LDFLAGS)

plq_topd_as_static: src/pf_query.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) $(LIBPATH) $(MACROS) -DTOPD -DPLQ_ONLY -DADAPTIVE_SCHED -DNO_ELASTIC $(OPTFLAGS) -o bin/plq_topd_as_static src/pf_query.cpp $(LDFLAGS)

plq_topd_as_iop: src/pf_query.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) $(LIBPATH) $(MACROS) -DTOPD -DPLQ_ONLY -DIOP -DADAPTIVE_SCHED $(OPTFLAGS) -o bin/plq_topd_as_iop src/pf_query.cpp $(LDFLAGS)

plq_topd_as_iop_static: src/pf_query.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) $(LIBPATH) $(MACROS) -DTOPD -DPLQ_ONLY -DIOP -DADAPTIVE_SCHED -DNO_ELASTIC $(OPTFLAGS) -o bin/plq_topd_as_iop_static src/pf_query.cpp $(LDFLAGS)

plq_topd_aspid: src/pf_query.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) $(LIBPATH) $(MACROS) -DTOPD -DPLQ_ONLY -DADAPTIVE_SCHED_PID $(OPTFLAGS) -o bin/plq_topd_aspid src/pf_query.cpp $(LDFLAGS)

plq_topd_aspid_static: src/pf_query.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) $(LIBPATH) $(MACROS) -DTOPD -DPLQ_ONLY -DADAPTIVE_SCHED_PID -DNO_ELASTIC $(OPTFLAGS) -o bin/plq_topd_aspid_static src/pf_query.cpp $(LDFLAGS)

plq_topd_aspid_iop: src/pf_query.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) $(LIBPATH) $(MACROS) -DTOPD -DPLQ_ONLY -DIOP -DADAPTIVE_SCHED_PID $(OPTFLAGS) -o bin/plq_topd_aspid_iop src/pf_query.cpp $(LDFLAGS)

plq_topd_aspid_iop_static: src/pf_query.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) $(LIBPATH) $(MACROS) -DTOPD -DPLQ_ONLY -DIOP -DADAPTIVE_SCHED_PID -DNO_ELASTIC $(OPTFLAGS) -o bin/plq_topd_aspid_iop_static src/pf_query.cpp $(LDFLAGS)

pf_skyline_pb: src/pf_query.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) $(LIBPATH) $(MACROS) -DSKYLINE -DPB_RR_SCHED $(OPTFLAGS) -o bin/pf_skyline_pb src/pf_query.cpp $(LDFLAGS)

pf_skyline_pb_static: src/pf_query.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) $(LIBPATH) $(MACROS) -DSKYLINE -DPB_RR_SCHED -DNO_ELASTIC $(OPTFLAGS) -o bin/pf_skyline_pb_static src/pf_query.cpp $(LDFLAGS)

pf_skyline_pb_iop: src/pf_query.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) $(LIBPATH) $(MACROS) -DSKYLINE -DIOP -DPB_RR_SCHED $(OPTFLAGS) -o bin/pf_skyline_pb_iop src/pf_query.cpp $(LDFLAGS)

pf_skyline_pb_iop_static: src/pf_query.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) $(LIBPATH) $(MACROS) -DSKYLINE -DIOP -DPB_RR_SCHED -DNO_ELASTIC $(OPTFLAGS) -o bin/pf_skyline_pb_iop_static src/pf_query.cpp $(LDFLAGS)

pf_skyline_tb: src/pf_query.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) $(LIBPATH) $(MACROS) -DSKYLINE -DTB_RR_SCHED $(OPTFLAGS) -o bin/pf_skyline_tb src/pf_query.cpp $(LDFLAGS)

pf_skyline_tb_static: src/pf_query.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) $(LIBPATH) $(MACROS) -DSKYLINE -DTB_RR_SCHED -DNO_ELASTIC $(OPTFLAGS) -o bin/pf_skyline_tb_static src/pf_query.cpp $(LDFLAGS)

pf_skyline_tb_iop: src/pf_query.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) $(LIBPATH) $(MACROS) -DSKYLINE -DIOP -DTB_RR_SCHED $(OPTFLAGS) -o bin/pf_skyline_tb_iop src/pf_query.cpp $(LDFLAGS)

pf_skyline_tb_iop_static: src/pf_query.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) $(LIBPATH) $(MACROS) -DSKYLINE -DIOP -DTB_RR_SCHED -DNO_ELASTIC $(OPTFLAGS) -o bin/pf_skyline_tb_iop_static src/pf_query.cpp $(LDFLAGS)

pf_skyline_as: src/pf_query.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) $(LIBPATH) $(MACROS) -DSKYLINE -DADAPTIVE_SCHED $(OPTFLAGS) -o bin/pf_skyline_as src/pf_query.cpp $(LDFLAGS)

pf_skyline_as_static: src/pf_query.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) $(LIBPATH) $(MACROS) -DSKYLINE -DADAPTIVE_SCHED -DNO_ELASTIC $(OPTFLAGS) -o bin/pf_skyline_as_static src/pf_query.cpp $(LDFLAGS)

pf_skyline_as_iop: src/pf_query.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) $(LIBPATH) $(MACROS) -DSKYLINE -DIOP -DADAPTIVE_SCHED $(OPTFLAGS) -o bin/pf_skyline_as_iop src/pf_query.cpp $(LDFLAGS)

pf_skyline_as_iop_static: src/pf_query.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) $(LIBPATH) $(MACROS) -DSKYLINE -DIOP -DADAPTIVE_SCHED -DNO_ELASTIC $(OPTFLAGS) -o bin/pf_skyline_as_iop_static src/pf_query.cpp $(LDFLAGS)

pf_skyline_aspid: src/pf_query.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) $(LIBPATH) $(MACROS) -DSKYLINE -DADAPTIVE_SCHED_PID $(OPTFLAGS) -o bin/pf_skyline_aspid src/pf_query.cpp $(LDFLAGS)

pf_skyline_aspid_static: src/pf_query.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) $(LIBPATH) $(MACROS) -DSKYLINE -DADAPTIVE_SCHED_PID -DNO_ELASTIC $(OPTFLAGS) -o bin/pf_skyline_aspid_static src/pf_query.cpp $(LDFLAGS)

pf_skyline_aspid_iop: src/pf_query.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) $(LIBPATH) $(MACROS) -DSKYLINE -DIOP -DADAPTIVE_SCHED_PID $(OPTFLAGS) -o bin/pf_skyline_aspid_iop src/pf_query.cpp $(LDFLAGS)

pf_skyline_aspid_iop_static: src/pf_query.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) $(LIBPATH) $(MACROS) -DSKYLINE -DIOP -DADAPTIVE_SCHED_PID -DNO_ELASTIC $(OPTFLAGS) -o bin/pf_skyline_aspid_iop_static src/pf_query.cpp $(LDFLAGS)

pf_topd_pb: src/pf_query.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) $(LIBPATH) $(MACROS) -DTOPD -DPB_RR_SCHED $(OPTFLAGS) -o bin/pf_topd_pb src/pf_query.cpp $(LDFLAGS)

pf_topd_pb_static: src/pf_query.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) $(LIBPATH) $(MACROS) -DTOPD -DPB_RR_SCHED -DNO_ELASTIC $(OPTFLAGS) -o bin/pf_topd_pb_static src/pf_query.cpp $(LDFLAGS)

pf_topd_pb_iop: src/pf_query.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) $(LIBPATH) $(MACROS) -DTOPD -DIOP -DPB_RR_SCHED $(OPTFLAGS) -o bin/pf_topd_pb_iop src/pf_query.cpp $(LDFLAGS)

pf_topd_pb_iop_static: src/pf_query.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) $(LIBPATH) $(MACROS) -DTOPD -DIOP -DPB_RR_SCHED -DNO_ELASTIC $(OPTFLAGS) -o bin/pf_topd_pb_iop_static src/pf_query.cpp $(LDFLAGS)

pf_topd_tb: src/pf_query.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) $(LIBPATH) $(MACROS) -DTOPD -DTB_RR_SCHED $(OPTFLAGS) -o bin/pf_topd_tb src/pf_query.cpp $(LDFLAGS)

pf_topd_tb_static: src/pf_query.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) $(LIBPATH) $(MACROS) -DTOPD -DTB_RR_SCHED -DNO_ELASTIC $(OPTFLAGS) -o bin/pf_topd_tb_static src/pf_query.cpp $(LDFLAGS)

pf_topd_tb_iop: src/pf_query.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) $(LIBPATH) $(MACROS) -DTOPD -DIOP -DTB_RR_SCHED $(OPTFLAGS) -o bin/pf_topd_tb_iop src/pf_query.cpp $(LDFLAGS)

pf_topd_tb_iop_static: src/pf_query.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) $(LIBPATH) $(MACROS) -DTOPD -DIOP -DTB_RR_SCHED -DNO_ELASTIC $(OPTFLAGS) -o bin/pf_topd_tb_iop_static src/pf_query.cpp $(LDFLAGS)

pf_topd_as: src/pf_query.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) $(LIBPATH) $(MACROS) -DTOPD -DADAPTIVE_SCHED $(OPTFLAGS) -o bin/pf_topd_as src/pf_query.cpp $(LDFLAGS)

pf_topd_as_static: src/pf_query.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) $(LIBPATH) $(MACROS) -DTOPD -DADAPTIVE_SCHED -DNO_ELASTIC $(OPTFLAGS) -o bin/pf_topd_as_static src/pf_query.cpp $(LDFLAGS)

pf_topd_as_iop: src/pf_query.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) $(LIBPATH) $(MACROS) -DTOPD -DIOP -DADAPTIVE_SCHED $(OPTFLAGS) -o bin/pf_topd_as_iop src/pf_query.cpp $(LDFLAGS)

pf_topd_as_iop_static: src/pf_query.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) $(LIBPATH) $(MACROS) -DTOPD -DIOP -DADAPTIVE_SCHED -DNO_ELASTIC $(OPTFLAGS) -o bin/pf_topd_as_iop_static src/pf_query.cpp $(LDFLAGS)

pf_topd_aspid: src/pf_query.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) $(LIBPATH) $(MACROS) -DTOPD -DADAPTIVE_SCHED_PID $(OPTFLAGS) -o bin/pf_topd_aspid src/pf_query.cpp $(LDFLAGS)

pf_topd_aspid_static: src/pf_query.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) $(LIBPATH) $(MACROS) -DTOPD -DADAPTIVE_SCHED_PID -DNO_ELASTIC $(OPTFLAGS) -o bin/pf_topd_aspid_static src/pf_query.cpp $(LDFLAGS)

pf_topd_aspid_iop: src/pf_query.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) $(LIBPATH) $(MACROS) -DTOPD -DIOP -DADAPTIVE_SCHED_PID $(OPTFLAGS) -o bin/pf_topd_aspid_iop src/pf_query.cpp $(LDFLAGS)

pf_topd_aspid_iop_static: src/pf_query.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) $(LIBPATH) $(MACROS) -DTOPD -DIOP -DADAPTIVE_SCHED_PID -DNO_ELASTIC $(OPTFLAGS) -o bin/pf_topd_aspid_iop_static src/pf_query.cpp $(LDFLAGS)

skyline_checker: src/skyline_checker.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -I ./spatialindex-src-1.8.1/include -L /usr/local/lib $(MACROS) $(OPTFLAGS) -o bin/skyline_checker src/skyline_checker.cpp -lspatialindex

all: $(TARGETS_GEN) $(TARGETS_PLQ) $(TARGETS_PF)

gen: $(TARGETS_GEN)

plq: $(TARGETS_PLQ)

pf: $(TARGETS_PF)

clean:
	rm -f bin/*

cleanall:
	\rm -f bin/*.o bin/*~
