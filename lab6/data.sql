INSERT INTO users (id, login, password_hash, first_name, last_name, created_at) VALUES
    (1, 'ivan_petrov', 'saltuser01$842c150cb25841d054ce81e3a3d6bb8da270b4d95e1516474fbded28811d31ba', 'Ivan', 'Petrov', '2026-04-01T09:00:00Z'),
    (2, 'anna_smirnova', 'saltuser02$66b2352400dacc191d443769c2ffa218c1aa6ead6401865ddea6fe6e7f92e57a', 'Anna', 'Smirnova', '2026-04-01T09:10:00Z'),
    (3, 'kirill_sokolov', 'saltuser03$95f92349de071280a2b4e58dd91ff79ac9b974265af981ad06141311564151eb', 'Kirill', 'Sokolov', '2026-04-01T09:20:00Z'),
    (4, 'elena_romanova', 'saltuser04$914a7f4b1b19aaf34474ad0499c19c57d9e783dc2933ace44f04e7669fd73fb1', 'Elena', 'Romanova', '2026-04-01T09:30:00Z'),
    (5, 'nikita_orlov', 'saltuser05$67f32ac2116142ccf961c34762bfa7f80f2ad66fc677f7136d55d6b273ea2a52', 'Nikita', 'Orlov', '2026-04-01T09:40:00Z'),
    (6, 'maria_lebedeva', 'saltuser06$ff4ea1edbf6e69a0b9b2255819e397c2ce70980f11d97f5a035aae83ecf6ca92', 'Maria', 'Lebedeva', '2026-04-01T09:50:00Z'),
    (7, 'oleg_kozlov', 'saltuser07$0fd49f475a81aa45b81201c5df2baf90af5e1bdc0be78357214df90e638a4d94', 'Oleg', 'Kozlov', '2026-04-01T10:00:00Z'),
    (8, 'alisa_volkova', 'saltuser08$7f388d2cc42543bec09d3b9130b0a5a7d71813056e04ed78b5c27dbf3a44b013', 'Alisa', 'Volkova', '2026-04-01T10:10:00Z'),
    (9, 'sergey_fedorov', 'saltuser09$a6cc5be268dfc3b990de96ef774c30e83775e629ced8f27898ffabe0a80c8035', 'Sergey', 'Fedorov', '2026-04-01T10:20:00Z'),
    (10, 'polina_vlasova', 'saltuser10$e948317940d06b8916b13fcdbc07849f67ee0398b2b04b0c799796d717607f91', 'Polina', 'Vlasova', '2026-04-01T10:30:00Z');

INSERT INTO hotels (id, name, city, address, stars, rooms_total, created_by, created_at) VALUES
    (1, 'Nevsky Grand Hotel', 'Saint Petersburg', 'Nevsky Prospect 12', 4, 40, 1, '2026-04-02T08:00:00Z'),
    (2, 'Moscow City Stay', 'Moscow', 'Presnenskaya Embankment 8', 5, 25, 2, '2026-04-02T08:10:00Z'),
    (3, 'Kazan Riverside', 'Kazan', 'Kremlevskaya 15', 4, 18, 3, '2026-04-02T08:20:00Z'),
    (4, 'Sochi Marina Resort', 'Sochi', 'Morskoy Lane 3', 5, 60, 4, '2026-04-02T08:30:00Z'),
    (5, 'Ural Business Hotel', 'Yekaterinburg', 'Lenina 55', 3, 30, 5, '2026-04-02T08:40:00Z'),
    (6, 'Volga View', 'Nizhny Novgorod', 'Rozhdestvenskaya 21', 4, 22, 6, '2026-04-02T08:50:00Z'),
    (7, 'Baltic Breeze', 'Kaliningrad', 'Mira Avenue 9', 4, 16, 7, '2026-04-02T09:00:00Z'),
    (8, 'Baikal Lights', 'Irkutsk', 'Sukhe-Batora 14', 5, 20, 8, '2026-04-02T09:10:00Z'),
    (9, 'Golden Ring Inn', 'Vladimir', 'Bolshaya Moskovskaya 18', 3, 14, 9, '2026-04-02T09:20:00Z'),
    (10, 'Arbat Rooms', 'Moscow', 'Old Arbat 27', 4, 12, 10, '2026-04-02T09:30:00Z');

INSERT INTO bookings (id, user_id, hotel_id, check_in, check_out, guests, status, created_at, cancelled_at) VALUES
    (1, 1, 2, '2026-04-10', '2026-04-14', 2, 'active', '2026-04-03T10:00:00Z', NULL),
    (2, 2, 1, '2026-04-12', '2026-04-16', 1, 'active', '2026-04-03T10:10:00Z', NULL),
    (3, 3, 3, '2026-04-15', '2026-04-18', 2, 'active', '2026-04-03T10:20:00Z', NULL),
    (4, 4, 4, '2026-04-20', '2026-04-26', 3, 'cancelled', '2026-04-03T10:30:00Z', '2026-04-05T12:00:00Z'),
    (5, 5, 5, '2026-04-11', '2026-04-13', 1, 'active', '2026-04-03T10:40:00Z', NULL),
    (6, 6, 6, '2026-04-25', '2026-04-30', 2, 'active', '2026-04-03T10:50:00Z', NULL),
    (7, 7, 7, '2026-04-28', '2026-05-02', 2, 'cancelled', '2026-04-03T11:00:00Z', '2026-04-08T09:00:00Z'),
    (8, 8, 8, '2026-05-03', '2026-05-08', 2, 'active', '2026-04-03T11:10:00Z', NULL),
    (9, 9, 9, '2026-05-10', '2026-05-14', 1, 'active', '2026-04-03T11:20:00Z', NULL),
    (10, 10, 10, '2026-05-15', '2026-05-19', 2, 'active', '2026-04-03T11:30:00Z', NULL);

SELECT setval('users_id_seq', (SELECT MAX(id) FROM users));
SELECT setval('hotels_id_seq', (SELECT MAX(id) FROM hotels));
SELECT setval('bookings_id_seq', (SELECT MAX(id) FROM bookings));
