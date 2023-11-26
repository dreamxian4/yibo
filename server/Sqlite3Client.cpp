#include "Sqlite3Client.h"
#include "Logger.h"

int CSqlite3Client::Connect(const KeyValue& args)
{
	auto it = args.find("host");
	if (it == args.end())return -1;
	if (m_db != NULL)return -2;
	int ret = sqlite3_open(it->second, &m_db);
	if (ret != 0) {
		TRACEE("connect failed:%d [%s]", ret, sqlite3_errmsg(m_db));
		return -3;
	}
	return 0;
}

int CSqlite3Client::Exec(const Buffer& sql)
{
	if (m_db == NULL)return -1;
	int ret = sqlite3_exec(m_db, sql, NULL, this, NULL);
	if (ret != SQLITE_OK) {
		TRACEE("sql={%s}", sql);
		TRACEE("Exec failed:%d [%s]", ret, sqlite3_errmsg(m_db));
		return -2;
	}
	return 0;
}

int CSqlite3Client::Exec(const Buffer& sql, Result& result, const _Table_& table)
{
	char* errmsg = NULL;
	if (m_db == NULL)return -1;
	ExecParam param(this, result, table);
	int ret = sqlite3_exec(m_db, sql,
		&CSqlite3Client::ExecCallback, (void*)&param, &errmsg);
	if (ret != SQLITE_OK) {
		TRACEE("sql={%s}", sql);
		TRACEE("Exec failed:%d [%s]", ret, errmsg);
		if (errmsg)sqlite3_free(errmsg);
		return -2;
	}
	if (errmsg)sqlite3_free(errmsg);
	return 0;
}

int CSqlite3Client::StartTransaction()
{
	if (m_db == NULL)return -1;
	int ret = sqlite3_exec(m_db, "BEGIN TRANSACTION", 0, 0, NULL);
	if (ret != SQLITE_OK) {
		TRACEE("sql={BEGIN TRANSACTION}");
		TRACEE("BEGIN failed:%d [%s]", ret, sqlite3_errmsg(m_db));
		return -2;
	}
	return 0;
}

int CSqlite3Client::CommitTransaction()
{
	if (m_db == NULL)return -1;
	int ret = sqlite3_exec(m_db, "COMMIT TRANSACTION", 0, 0, NULL);
	if (ret != SQLITE_OK) {
		TRACEE("sql={COMMIT TRANSACTION}");
		TRACEE("COMMIT failed:%d [%s]", ret, sqlite3_errmsg(m_db));
		return -2;
	}
	return 0;
}

int CSqlite3Client::RollbackTransaction()
{
	if (m_db == NULL)return -1;
	int ret = sqlite3_exec(m_db, "ROLLBACK TRANSACTION", 0, 0, NULL);
	if (ret != SQLITE_OK) {
		TRACEE("sql={ROLLBACK TRANSACTION}");
		TRACEE("ROLLBACK failed:%d [%s]", ret, sqlite3_errmsg(m_db));
		return -2;
	}
	return 0;
}

int CSqlite3Client::Close()
{
	if (m_db == NULL)return -1;
	int ret = sqlite3_close(m_db);
	if (ret != SQLITE_OK) {
		TRACEE("Close failed:%d [%s]", ret, sqlite3_errmsg(m_db));
		return -2;
	}
	m_db = NULL;
	return 0;
}

bool CSqlite3Client::IsConnected()
{
	return m_db != NULL;
}

int CSqlite3Client::ExecCallback(void* arg, int count, char** names, char** values)
{
	ExecParam* param = (ExecParam*)arg;
	return param->obj->ExecCallback(param->result, param->table, count, names, values);
}

int CSqlite3Client::ExecCallback(Result& result, const _Table_& table, int count, char** names, char** values)
{
	PTable pTable = table.Copy();
	if (pTable == nullptr) {
		TRACEE("table %s error!", (const char*)(Buffer)table);
		return -1;
	}
	for (int i = 0; i < count; i++) {
		Buffer name = names[i];
		auto it = pTable->Fields.find(name);
		if (it == pTable->Fields.end()) {
			TRACEE("table %s error!", (const char*)(Buffer)table);
			return -2;
		}
		if (values[i] != NULL)
			it->second->LoadFromStr(values[i]);
	}
	result.push_back(pTable);
	return 0;
}

_sqlite3_table_::_sqlite3_table_(const _sqlite3_table_& table)
{
	Database = table.Database;
	Name = table.Name;
	for (size_t i = 0; i < table.FieldDefine.size(); i++)
	{
		PField field = PField(new _sqlite3_field_(*
			(_sqlite3_field_*)table.FieldDefine[i].get()));
		FieldDefine.push_back(field);
		Fields[field->Name] = field;
	}
}

Buffer _sqlite3_table_::Create()
{	//CREATE TABLE IF NOT EXISTS ��ȫ�� (�ж���,����);
	//��ȫ�� = ���ݿ�.����
	Buffer sql = "CREATE TABLE IF NOT EXISTS " + (Buffer)*this + "(\r\n";
	for (size_t i = 0; i < FieldDefine.size(); i++) {
		if (i > 0)sql += ",";
		sql += FieldDefine[i]->Create();
	}
	sql += ");";
	TRACEI("sql = %s", (char*)sql);
	return sql;
}

Buffer _sqlite3_table_::Drop()
{
	Buffer sql = "DROP TABLE " + (Buffer)*this + ";";
	TRACEI("sql = %s", (char*)sql);
	return sql;
}

Buffer _sqlite3_table_::Insert(const _Table_& values)
{	//INSERT INTO ��ȫ�� (��1,...,��n)
	//VALUES(ֵ1,...,ֵn);
	Buffer sql = "INSERT INTO " + (Buffer)*this + " (";
	bool isfirst = true;
	for (size_t i = 0; i < FieldDefine.size(); i++) {
		if (FieldDefine[i]->Condition & SQL_INSERT) {
			if (!isfirst)sql += ",";
			else isfirst = false;
			sql += (Buffer)*FieldDefine[i];
		}
	}
	sql += ") VALUES(";
	isfirst = true;
	for (size_t i = 0; i < FieldDefine.size(); i++) {
		if (FieldDefine[i]->Condition & SQL_INSERT) {
			if (!isfirst)sql += ",";
			else isfirst = false;
			sql += FieldDefine[i]->toSqlStr();
		}
	}
	sql += " );";
	TRACEI("sql = %s", (char*)sql);
	return sql;
}

Buffer _sqlite3_table_::Delete(const _Table_& values)
{// DELETE FROM ��ȫ�� WHERE ����
	Buffer sql = "DELETE FROM " + (Buffer)*this + " ";
	Buffer Where = "";
	bool isfirst = true;
	for (size_t i = 0; i < FieldDefine.size(); i++) {
		if (FieldDefine[i]->Condition & SQL_CONDITION) {
			if (!isfirst)Where += " AND ";
			else isfirst = false;
			Where += (Buffer)*FieldDefine[i] + "=" + FieldDefine[i]->toSqlStr();
		}
	}
	if (Where.size() > 0)
		sql += " WHERE " + Where;
	sql += ";";
	TRACEI("sql = %s", (char*)sql);
	return sql;
}

Buffer _sqlite3_table_::Query()
{//SELECT ����1 ,����2 ,... ,����n FROM ��ȫ��;
	Buffer sql = "SELECT ";
	for (size_t i = 0; i < FieldDefine.size(); i++)
	{
		if (i > 0)sql += ',';
		sql += '"' + FieldDefine[i]->Name + "\" ";
	}
	sql += " FROM " + (Buffer)*this + ";";
	TRACEI("sql = %s", (char*)sql);
	return sql;
}

PTable _sqlite3_table_::Copy() const
{
	return PTable(new _sqlite3_table_(*this));
}

void _sqlite3_table_::ClearFieldUsed()
{
	for (size_t i = 0; i < FieldDefine.size(); i++) {
		FieldDefine[i]->Condition = 0;
	}
}

_sqlite3_table_::operator const Buffer() const
{
	Buffer Head;
	if (Database.size())
		Head = '"' + Database + "\".";
	return Head + '"' + Name + '"';
}
