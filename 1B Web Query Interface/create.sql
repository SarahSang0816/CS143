CREATE TABLE Movie (
	-- id is the primary key of Movie
	id INT PRIMARY KEY,
	-- title can't be NULL
	title VARCHAR(100) NOT NULL,
	year INT,
	rating VARCHAR(10),
	company VARCHAR(50) NOT NULL,
	-- id must be non-negative
	CHECK (id >= 0),
	-- year must be positive
	CHECK (year > 0),
	-- rating must be an MPAA rating (NR means not rated)
	CHECK (rating IN ('NR', 'G', 'PG', 'PG-13', 'R', 'NC-17')) )
	ENGINE = INNODB,
	CHARACTER SET utf8;

CREATE TABLE Actor (
	-- id is the primary key of Actor
	id INT PRIMARY KEY,
	last VARCHAR(20),
	first VARCHAR(20),
	sex VARCHAR(6),
	-- date of birth can't be NULL
	dob DATE NOT NULL,
	dod DATE,
	-- id must be non-negative
	CHECK (id >= 0),
	-- sex must be male or female
	CHECK (sex = 'Male' OR sex = 'Female') )
	ENGINE = INNODB,
	CHARACTER SET utf8;

CREATE TABLE Director (
	-- id is the primary key of Director
	id INT PRIMARY KEY,
	last VARCHAR(20),
	first VARCHAR(20),
	-- date of birth can't be NULL
	dob DATE NOT NULL,
	dod DATE,
	-- id must be non-negative
	CHECK (id >= 0) )
	ENGINE = INNODB,
	CHARACTER SET utf8;

CREATE TABLE MovieGenre (
	-- mid must be one of the ids listed in Movie
	-- not bothering with a check constraint here b/c Movie has it
	mid INT,
	-- genre can't be NULL
	genre VARCHAR(20) NOT NULL,
	FOREIGN KEY (mid) REFERENCES Movie(id) )
	ENGINE = INNODB,
	CHARACTER SET utf8;



CREATE TABLE MovieDirector (
       mid INT,
       did INT,
       FOREIGN KEY (mid) REFERENCES Movie(id),
       FOREIGN KEY (did) REFERENCES Director(id))
       ENGINE = INNODB,
       CHARACTER SET utf8;


CREATE TABLE MovieActor (
       mid INT,
       aid INT,
       role VARCHAR(50),
       FOREIGN KEY (mid) REFERENCES Movie(id),
       FOREIGN KEY (aid) REFERENCES Actor(id) )
       ENGINE = INNODB,
       CHARACTER SET utf8;


CREATE TABLE Review (
       name VARCHAR(20),
       time TIMESTAMP,
       mid INT,
       rating INT,
       comment VARCHAR(500),
       -- rating should be between [1-5]
       CHECK (rating > 0 AND  rating <= 5),
       FOREIGN KEY (mid) REFERENCES Movie(id) )
       ENGINE = INNODB,
       CHARACTER SET utf8;


CREATE TABLE MaxPersonID (
       id INT PRIMARY KEY
)
ENGINE = INNODB;


CREATE TABLE MaxMovieID (
       id INT PRIMARY KEY
)
ENGINE = INNODB;




