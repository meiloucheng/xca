#include "KeyDB.h"
#include <stdio.h>


KeyDB::KeyDB(QString DBfile, QListBox *list, 
	QObject *parent , const char *name = 0)
	:QObject( parent, name)
{
	listView = list;
	dbenv = new DbEnv(DB_CXX_NO_EXCEPTIONS | DB_INIT_TXN);
	data = new Db(dbenv, 0);
	cerr << "DB:" << DBfile <<"\n";//
	int x;
	if ( x = data->open(DBfile,"keydb",DB_BTREE, DB_CREATE,0600)) 
		data->err(x,"DB open");
	//cerr << "before Update\n";
	updateView();
	//cerr << "after Update\n";
}


KeyDB::~KeyDB()
{
	data->close(0);
//	dbenv->close(0);
}



bool KeyDB::updateView()
{
	listView->clear();
	Dbc *cursor;
	if (int x = data->cursor(NULL, &cursor, 0))
		data->err(x,"DB new Cursor");
	Dbt *key = new Dbt();
	Dbt *data = new Dbt();
	QString desc;
	//cerr << "before While loop\n";
	while (!cursor->get(key, data, DB_NEXT)) {
		//cerr << "in loop\n";
		desc = (char *)key->get_data();
		listView->insertItem(desc);
	}
}


bool KeyDB::insertKey(RSAkey *key) 
{
	QString desc = key->description();
	QString orig = desc;
	int size=0;
	unsigned char *p;
	p = key->getKey(&size);
	int cnt=1;
	int x = DB_KEYEXIST;
	while (x == DB_KEYEXIST) {
	   Dbt k((void *)desc.latin1(), desc.length()+1);
	   Dbt d((void *)p, size);
           cerr << "Size: " << d.get_size() << "\n";
	
	   if (x = data->put(NULL, &k, &d, DB_NODUPDATA | DB_NOOVERWRITE)) {
		data->err(x,"DB Error put");
	   }
	   desc = orig + "_" + QString::number(cnt++);
	}
	if (x != DB_KEYEXIST && x != 0) {
	   data->err(x,"DB Error put");
	   return false;
	}
	OPENSSL_free(p);
	updateView();
	return true;
}


bool KeyDB::deleteKey(RSAkey *key) 
{
	QString desc = key->description();
	Dbt k((void *)desc.latin1(), desc.length()+1);
	int x = data->del(NULL, &k, 0);
	updateView();
	if (x){
	   data->err(x,"DB Error del");
	   return false;
	}
	return true;
}

bool KeyDB::updateKey(RSAkey *key, QString desc) 
{
	if (deleteKey(key)){
	   key->setDescription(desc);
	   return insertKey(key);
	}
	return true;
}


RSAkey *KeyDB::getSelectedKey()
{
	unsigned char *p;
	RSAkey *targetKey = NULL;
	QString desc = listView->currentText();
	Dbt k((void *)desc.latin1(), desc.length()+1);
	Dbt d((void *)p, 0);
	int x = data->get(NULL, &k, &d, 0);
	p = (unsigned char *)d.get_data();
	int size = d.get_size();
	if (x) data->err(x,"DB Error get");
	else targetKey = new RSAkey(p, size);
	targetKey->setDescription(desc);
	return targetKey;
}