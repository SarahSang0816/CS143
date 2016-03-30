-- Select the names of all actors in 'Die Another Day'
SELECT CONCAT(first, ' ', last) AS Name
FROM MovieActor, Movie, Actor
WHERE MovieActor.mid = Movie.id AND title = 'Die Another Day' AND Actor.id = MovieActor.aid;

-- Get a count of all actors who acted in multiple movies
SELECT COUNT(*) AS Total
FROM (SELECT aid FROM MovieActor GROUP BY aid
	HAVING COUNT(mid) > 1) ids;

-- Select the titles of all movies which are both 'Comedy' and 'Action'
SELECT title FROM (SELECT mid FROM MovieGenre WHERE genre = 'Comedy') C, 
	(SELECT mid FROM MovieGenre WHERE genre = 'Action') A, Movie
	WHERE C.mid = A.mid AND A.mid = id;
