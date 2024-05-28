#pragma once
#include "Public.h"
#include "DatabaseHelper.h"
#include "sqlite3/sqlite3.h"

class CSqlite3Client
	:public CDatabaseClient
{
public:
	CSqlite3Client(const CSqlite3Client&) = delete;
	CSqlite3Client& operator=(const CSqlite3Client&) = delete;
public:
	CSqlite3Client() {
		m_db = NULL;
		m_stmt = NULL;
	}
	virtual ~CSqlite3Client() {
		Close();
	}
public:
	//����
	virtual int Connect(const KeyValue& args);
	//ִ��
	virtual int Exec(const Buffer& sql);
	//�������ִ��
	virtual int Exec(const Buffer& sql, Result& result, const _Table_& table);
	//��������
	virtual int StartTransaction();
	//�ύ����
	virtual int CommitTransaction();
	//�ع�����
	virtual int RollbackTransaction();
	//�ر�����
	virtual int Close();
	//�Ƿ����� true��ʾ������ false��ʾδ����
	virtual bool IsConnected();
private:
	static int ExecCallback(void* arg, int count, char** names, char** values);
	int ExecCallback(Result& result, const _Table_& table, int count, char** names, char** values);
private:
	sqlite3_stmt* m_stmt;
	sqlite3* m_db;
private:
	class ExecParam {
	public:
		ExecParam(CSqlite3Client* obj, Result& result, const _Table_& table)
			:obj(obj), result(result), table(table)
		{}
		CSqlite3Client* obj;
		Result& result;
		const _Table_& table;
	};
};

class _sqlite3_table_ :
	public _Table_
{
public:
	_sqlite3_table_() :_Table_() {}
	_sqlite3_table_(const _sqlite3_table_& table);
	virtual ~_sqlite3_table_();
	//���ش�����SQL���
	virtual Buffer Create();
	//ɾ����
	virtual Buffer Drop();
	//��ɾ�Ĳ�
	//TODO:���������Ż�
	virtual Buffer Insert(const _Table_& values);
	virtual Buffer Delete(const _Table_& values);
	//TODO:���������Ż�
	virtual Buffer Modify(const _Table_& values);
	virtual Buffer Query(const Buffer& condition = "");
	//����һ�����ڱ�Ķ���
	virtual PTable Copy()const;
	virtual void ClearFieldUsed();
public:
	//��ȡ���ȫ��
	virtual operator const Buffer() const;
};

class _sqlite3_field_ :
	public _Field_
{
public:
	_sqlite3_field_();
	_sqlite3_field_(
		int ntype,
		const Buffer& name,
		unsigned attr,
		const Buffer& type,
		const Buffer& size,
		const Buffer& default_,
		const Buffer& check
	);
	_sqlite3_field_(const _sqlite3_field_& field);
	virtual ~_sqlite3_field_();
	virtual Buffer Create();
	virtual void LoadFromStr(const Buffer& str);
	//where ���ʹ�õ�
	virtual Buffer toEqualExp() const;
	virtual Buffer toSqlStr() const;
	//�е�ȫ��
	virtual operator const Buffer() const;
private:
	Buffer Str2Hex(const Buffer& data) const;
};

#define DECLARE_TABLE_CLASS(name, base) class name:public base { \
public: \
virtual PTable Copy() const {return PTable(new name(*this));} \
name():base(){Name=#name;

#define DECLARE_FIELD(ntype,name,attr,type,size,default_,check) \
{PField field(new _sqlite3_field_(ntype, #name, attr, type, size, default_, check));FieldDefine.push_back(field);Fields[#name] = field; }

#define DECLARE_TABLE_CLASS_EDN() }};