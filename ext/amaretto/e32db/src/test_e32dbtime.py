# Copyright (c) 2005 Nokia Corporation
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.


import e32db

def query(db,statement):
  DBV = e32db.Db_view()
  DBV.prepare(db, unicode(statement))
  n = DBV.count_line()
  DBV.first_line()
  data=[]
  for i in xrange(n):
    DBV.get_line()
    line=[]
    for j in xrange(DBV.col_count()):
      line.append(DBV.col(1+j))
      t=DBV.col_type(1+j)
      print "Column type: %d"%t
      if t != 15:
        print "Raw value: %s"%repr(DBV.col_raw(1+j))
      if t==10:
        print "Raw time: %s"%repr(DBV.col_rawtime(1+j))
        print "Formatted: %s"%e32db.format_rawtime(DBV.col_rawtime(1+j))
        
    data.append(tuple(line))
    DBV.next_line()
  del DBV
  return data

def dbexec(statement):
  print "Executing %s"%statement
  db.execute(unicode(statement))

db=e32db.Dbms()
FILE=u'c:\\timedb'
if os.path.isfile(FILE):
	os.remove(FILE)
db.create(FILE)
db.open(FILE)
db.execute(u'create table data (a time, b timestamp, c date)')

timeval=12345678;

fmt=e32db.format_time

dbexec("insert into data values(#%s#, #%s#, #%s#)"%(fmt(timeval),
                                                    fmt(timeval),
                                                    fmt(timeval)))
print query(db,u'select * from data')

print "Closing database."
db.close()

import appuifw
formdata = [(u"Date", 'date', float(timeval)),
            (u"Time", 'time', float(timeval))]
flags = appuifw.FFormEditModeOnly
print formdata
form=appuifw.Form(formdata,flags)
form.execute()

for row in form:
  print row

print "calling form again with same values:"
form.execute()
for row in form:
  print row


