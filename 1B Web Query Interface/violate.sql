-- Three Primary Key Constraints

-- 1. Inserting two Movies with the same id (primary key)
-- id 272 has already exixted in Movie Table
-- ERROR 1062 (23000) at line 6: Duplicate entry '272' for key 'PRIMARY'
INSERT INTO Movie VALUES (272,"Baby Take a Bow",1934,"PG","Fox Film Corporation");

-- 2. Inserting two Actors with the same id (primary key)
-- id 1 has already existed in Actor Tabble
-- ERROR 1062 (23000) at line 11: Duplicate entry '1' for key 'PRIMARY'
INSERT INTO Actor VALUES (1, 'Mike', 'James', '1982-08-08', '2100-01-01');

-- 3. Inserting two Directors with the same id (primary key)
-- id 37146 has already existed in Director Table
-- ERROR 1062 (23000) at line 16: Duplicate entry '37146' for key 'PRIMARY'
INSERT INTO Director VALUES (37146, 'Anna', 'Adam', '1962-08-08', '2090-01-01');


-- Six referential integrity constraints

-- 1. MovieGenre : mid in MovieGenre should appear in Moive id
-- ERROR 1452 (23000) at line 24: Cannot add or update a child row: a foreign key constraint fails 
-- `CS143`.`MovieGenre`, CONSTRAINT `MovieGenre_ibfk_1` FOREIGN KEY (`mid`) REFERENCES `Movie` (`id`))
INSERT INTO MovieGenre VALUES (-1, "Action");

-- 2. MovieDirector : mid in MovieDirector should appear in Moive id
-- ERROR 1452 (23000) at line 29: Cannot add or update a child row: a foreign key constraint fails 
-- (`CS143`.`MovieDirector`, CONSTRAINT `MovieDirector_ibfk_1` FOREIGN KEY (`mid`) REFERENCES `Movie` (`id`))
INSERT INTO MovieDirector VALUES (-9999, 37146);

-- 3. MovieDirector : id's cannot be less than zero
-- ERROR 1452 (23000) at line 34: Cannot add or update a child row: a foreign key constraint fails 
-- `CS143`.`MovieDirector`, CONSTRAINT `MovieDirector_ibfk_2` FOREIGN KEY (`did`) REFERENCES `Director` (`id`))
INSERT INTO MovieDirector VALUES (272, -1);

-- 4. MovieActor : mid in MovieActor should appear in Movie id
-- ERROR 1452 (23000) at line 39: Cannot add or update a child row: a foreign key constraint fails 
-- (`CS143`.`MovieActor`, CONSTRAINT `MovieActor_ibfk_1` FOREIGN KEY (`mid`) REFERENCES `Movie` (`id`))
INSERT INTO MovieActor VALUES (-1, 10, "Anna");

-- 5. MovieActor : aid in MovieActor should appear in Actor id
-- ERROR 1452 (23000) at line 44: Cannot add or update a child row: a foreign key constraint fails 
-- `CS143`.`MovieActor`, CONSTRAINT `MovieActor_ibfk_2` FOREIGN KEY (`aid`) REFERENCES `Actor` (`id`))
INSERT INTO MovieActor VALUES (272, -1, "Jenny");

-- 6. Review : id's cannot be less than zero
-- ERROR 1452 (23000) at line 49: Cannot add or update a child row: a foreign key constraint fails 
`TEST`.`Review`, CONSTRAINT `Review_ibfk_1` FOREIGN KEY (`mid`) REFERENCES `Movie` (`id`))
INSERT INTO Review VALUES ('jsmith', '2011-03-23 03:14:07', -1, 4, 'Cool movie');


-- Three CHECK constraints

-- 1. Check that the Movie id is greater than zero
INSERT INTO Movie VALUES (-20,"Baby Take a Bow",1934,"PG","Fox Film Corporation");

-- 2. Check that the Actor's sex is 'Male' or 'Female'
INSERT INTO Actor VALUES (1,"A","Isabelle","Homosexual",'1975-05-25',\N);

-- 3. Check that the Director's id is greater than zero
INSERT INTO Director VALUES (-200,"Levitow","Abe",'1922-07-02','1975-05-08');


