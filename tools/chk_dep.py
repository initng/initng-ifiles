#!/usr/bin/python
# -*- coding:koi8-r -*-

# Copyright 2005 Sergey A. Saukh <thelich@yandex.ru>
#
# chk_dep is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# chk_dep is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with the sofware; see the file COPYING. If not, write to the
# Free Software Foundation, Inc.,
# 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
#

import os,sys,optparse

initng_dir='/etc/initng'
dir_list=['','system','daemon','net']
runlevel='default'
service_db={}

parser=optparse.OptionParser(usage='%prog [options] [runlevel] ',version='%prog 1.1')
parser.add_option('-v','--verbose',dest='verbose',help='Be verbose',default=False,action='store_true')
parser.add_option('-s','--without-summary',dest='summary',help='Do not print summary',default=True,action='store_false')

(options,args)=parser.parse_args()

def rl_parse(name,infile):
    deps=[]
    for line in infile:
	if line.startswith('#') or not line.strip():
	    continue

	deps+=line.split()

    service_db[name]=[deps,0]
    infile.close()

def parse_i_token(name,line):
    arg_list=''
    if line.startswith(name):
	arg_list=line[len(name):].strip()
	if arg_list.startswith('='):
	    arg_list=arg_list[1:]
		
	comma=arg_list.find(';')
		
	if comma>=0:
	    arg_list=arg_list[:comma]

    return arg_list.split()

def i_parse(infile):
    deps=[]
    brackets=0
    is_service=False
    for line in infile:
	if line.startswith('#') or not line.strip():
	    continue

	line=line.strip()

	brackets+=line.count('{')
	brackets-=line.count('}')

	if brackets==1:
	    if not is_service:
		service_type=[x for x in ['service','daemon','virtual','class'] if line.startswith(x)]
		if service_type:
		    name=line[len(service_type[0]):line.find('{')]

		    comma=name.find(':')
		    if comma>=0:
			deps=[name[comma+1:].strip()]
			name=name[:comma]

		    name=name.strip()
		    is_service=True
		continue

	    new_deps=parse_i_token('need',line)
	    if new_deps:
		deps+=new_deps
		continue

	    provide_list=parse_i_token('provide',line)
	    for provide in provide_list:
		if not service_db.has_key(provide):
		    service_db[provide]=[[name],0]

	elif brackets==0:
	    if is_service:
		service_db[name]=[deps,0]
		deps=[]
		is_service=False

    infile.close()

def parse_deps(name):

    if not service_db.has_key(name):
	infile,srv_type=open_srv_file(name)
	if infile:
	    if srv_type==1:
		rl_parse(name,infile)
	    else:
		i_parse(infile)

	    if service_db.has_key(name):
		for dep in service_db[name][0]:
		    parse_deps(dep)
	    else:
		slash=name.rfind('/')
		new_name=name[:slash]+'/*'
		if service_db.has_key(new_name):
		    service_db[name]=service_db[new_name]
		else:
		    service_db[name]=[[],0]
	else:
	    service_db[name]=[[],4]

def check_deps(name,dep_on_me):
    total_result=True
    if options.verbose:
	print ' '*len(dep_on_me)+'Checking %s...'%name
    if service_db.has_key(name):
	state=service_db[name][1]
	if state==0:
	    for dep in service_db[name][0]:
		if dep in dep_on_me:
		    if options.verbose:
			print >> sys.stderr, ' '*len(dep_on_me)+'Service %s and %s - cyclic dependency'%(name,dep)
		    service_db[name].append(dep)
		    service_db[name][1]=3
		    result=False
		else:
		    result=check_deps(dep,dep_on_me+[name])
		    
		total_result = total_result and result
	elif state!=1:
	    total_result=False
    else:
	if options.verbose:
	    print ' '*len(dep_on_me)+'%s: not found'%name
	return False

    if total_result:
	if options.verbose:
	    print ' '*len(dep_on_me)+'%s: ok'%name
	service_db[name][1]=1
    else:
	if options.verbose:
	    print ' '*len(dep_on_me)+'%s: not ok'%name
	if service_db[name][1]==0:
	    service_db[name][1]=2

    return total_result

def open_srv_file(name):
    infile=None
    filename=os.path.join(initng_dir,name+'.runlevel')
    try:
	infile=open(filename)
    except:
	pass
    else:
	return infile,1

    infile=open_i_file(name)
    if not infile:
	slash=name.rfind('/')
	if slash>=0:
	    infile=open_i_file(name[:slash])

    return infile,2

def open_i_file(name):
    for dir_name in dir_list:
	filename=os.path.join(initng_dir,dir_name,name+'.i')
	try:
	    infile=open(filename)
	except:
	    continue
	else:
	    return infile

    return None

if args[0:]:
    runlevel=args[0]

parse_deps(runlevel)
if service_db[runlevel][1]==3:
    print >> sys.stderr, 'No file for runlevel %s exists'%runlevel
    sys.exit(1)

if not service_db[runlevel][0]:
    print >> sys.stderr, 'No services in runlevel %s'%runlevel
    sys.exit(2)

check_deps(runlevel,[])
if options.summary:
    if service_db[runlevel][1]!=1:
	print 'Runlevel %s not ok because:'%runlevel
	for key in service_db.keys():
	    if service_db[key][1]==3:
		print 'Service %s has cyclic dependency with %s'%(key, service_db[key][2])
	    elif service_db[key][1]==4:
		print 'No file fo service %s is found'%key

    else:
	print 'Runlevel %s is ok. No problems with dependencies found'%runlevel