#pragma once
#pragma once
#include "Public.h"
#include "DatabaseHelper.h"
#include <mysql/mysql.h>
class CMysqlClient
	:public CDatabaseClient
{
public:
	CMysqlClient(const CMysqlClient&) = delete;
	CMysqlClient& operator=(const CMysqlClient&)= delete;
public:
	CMysqlClient() {
		bzero(&m_db, sizeof(m_db));
		m_bInit = false;
	}
	virtual ~CMysqlClient() {
		Close();
	}
public:
	//����
	virtual int Connect(const KeyValue& args);
	//ִ��
	virtual int Exec(const Buffer& sql);
	//�������ִ��
	virtual int Exec(const Buffer& sql, Result&result, const _Table_& table);
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
	MYSQL m_db;
	bool m_bInit;//Ĭ����false ��ʾû�г�ʼ�� ��ʼ��֮����Ϊtrue����ʾ�Ѿ�����
private:
	class ExecParam {
	public:
		ExecParam(CMysqlClient* obj, Result&
			result, const _Table_& table)
			:obj(obj), result(result),
			table(table)
		{}
		CMysqlClient* obj;
		Result& result;
		const _Table_& table;
	};
};
class _mysql_table_ :
	public _Table_
{
public:
	_mysql_table_() :_Table_() {}
	_mysql_table_(const _mysql_table_& table);
	virtual ~_mysql_table_();
	//���ش�����SQL���
	virtual Buffer Create();
	//ɾ����
	virtual Buffer Drop();
	//��ɾ�Ĳ�
	//TODO:���������Ż�
	virtual Buffer Insert(const _Table_&values);
	virtual Buffer Delete(const _Table_&values);
	//TODO:���������Ż�
	virtual Buffer Modify(const _Table_&values);
	virtual Buffer Query(const Buffer& condition = "");
	//����һ�����ڱ�Ķ���
	virtual PTable Copy()const;
	virtual void ClearFieldUsed();
public:
	//��ȡ���ȫ��
	virtual operator const Buffer() const;
};
class _mysql_field_ :
	public _Field_
{
public:
	_mysql_field_();
	_mysql_field_(
		int ntype,
		const Buffer& name,
		unsigned attr,
		const Buffer& type,
		const Buffer& size,
		const Buffer& default_,
		const Buffer& check
	);
	_mysql_field_(const _mysql_field_& field);
	virtual ~_mysql_field_();
	virtual Buffer Create();
	virtual void LoadFromStr(const Buffer& str);
	//where ���ʹ�õ�
	virtual Buffer toEqualExp() const;
	virtual Buffer toSqlStr() const;
	//�е�ȫ��
	virtual operator const Buffer() const;
private:
	Buffer Str2Hex(const Buffer& data) const;
	union {
		bool Bool;
		int Integer;
		double Double;
		Buffer* String;
	}Value;
	int nType;
};
#define DECLARE_TABLE_CLASS(name, base) class name :public base{ \
public: \
	virtual PTable Copy() const {\
	return PTable(new\
		name(*this));} \
name() :base() {\
	Name = #name;
#define DECLARE_MYSQL_FIELD(ntype, name, attr, type, size, default_, check) \
	{PField field(new _mysql_field_(ntype, #name,attr, type, size, default_,check)); FieldDefine.push_back(field); Fields[#name] = field; }

#define DECLARE_TABLE_CLASS_EDN() }};