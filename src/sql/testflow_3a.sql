SET search_path TO test0_6_1;
truncate torder;
truncate tstack;
truncate tmvt;
truncate towner;
SELECT setval('tstack_id_seq',1,false);

copy torder from '/home/olivier/ob92/src/sql/torder_test_10000.sql';
copy towner from '/home/olivier/ob92/src/sql/towner_test_10000.sql';
truncate tstack;
SELECT setval('tstack_id_seq',10000,true);

select * from fsubmitorder(5,'own82',NULL,'qlt22',1,'qlt23',1,1);select * from fproducemvt();
select * from fsubmitorder(1,'own82',NULL,'qlt22',67432,'qlt23',30183,30183);select * from fproducemvt();

select * from fsubmitorder(6,'own82',NULL,'qlt22',1,'qlt23',1,1);select * from fproducemvt(); 
select * from fsubmitorder(2,'own82',NULL,'qlt22',61017,'qlt23',45276,45276);select * from fproducemvt();




