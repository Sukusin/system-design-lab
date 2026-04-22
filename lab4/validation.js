const dbName = 'hotel_booking_mongo';
db = db.getSiblingDB(dbName);

function createOrUpdateCollection(name, options) {
  const exists = db.getCollectionInfos({ name }).length > 0;
  if (!exists) {
    db.createCollection(name, options);
  } else {
    db.runCommand({
      collMod: name,
      validator: options.validator,
      validationLevel: options.validationLevel,
      validationAction: options.validationAction,
    });
  }
}

createOrUpdateCollection('users', {
  validator: {
    $jsonSchema: {
      bsonType: 'object',
      required: ['login', 'password_hash', 'first_name', 'last_name', 'created_at'],
      additionalProperties: true,
      properties: {
        _id: { bsonType: 'objectId' },
        login: {
          bsonType: 'string',
          minLength: 3,
          maxLength: 50,
          pattern: '^[a-zA-Z0-9_\\.-]+$'
        },
        password_hash: { bsonType: 'string', minLength: 20 },
        first_name: { bsonType: 'string', minLength: 1, maxLength: 100 },
        last_name: { bsonType: 'string', minLength: 1, maxLength: 100 },
        created_at: { bsonType: 'date' }
      }
    }
  },
  validationLevel: 'strict',
  validationAction: 'error'
});

createOrUpdateCollection('hotels', {
  validator: {
    $jsonSchema: {
      bsonType: 'object',
      required: ['name', 'city', 'address', 'stars', 'rooms_total', 'amenities', 'created_by', 'created_at'],
      additionalProperties: true,
      properties: {
        _id: { bsonType: 'objectId' },
        name: { bsonType: 'string', minLength: 2, maxLength: 200 },
        city: { bsonType: 'string', minLength: 2, maxLength: 100 },
        address: { bsonType: 'string', minLength: 5, maxLength: 255 },
        stars: { bsonType: 'int', minimum: 1, maximum: 5 },
        rooms_total: { bsonType: 'int', minimum: 1, maximum: 10000 },
        amenities: {
          bsonType: 'array',
          minItems: 1,
          items: { bsonType: 'string' }
        },
        created_by: { bsonType: 'objectId' },
        created_at: { bsonType: 'date' }
      }
    }
  },
  validationLevel: 'strict',
  validationAction: 'error'
});

createOrUpdateCollection('bookings', {
  validator: {
    $and: [
      {
        $jsonSchema: {
          bsonType: 'object',
          required: ['user_id', 'hotel_id', 'hotel_snapshot', 'stay', 'status', 'created_at', 'status_history'],
          additionalProperties: true,
          properties: {
            _id: { bsonType: 'objectId' },
            user_id: { bsonType: 'objectId' },
            hotel_id: { bsonType: 'objectId' },
            hotel_snapshot: {
              bsonType: 'object',
              required: ['name', 'city', 'address', 'stars'],
              properties: {
                name: { bsonType: 'string', minLength: 2, maxLength: 200 },
                city: { bsonType: 'string', minLength: 2, maxLength: 100 },
                address: { bsonType: 'string', minLength: 5, maxLength: 255 },
                stars: { bsonType: 'int', minimum: 1, maximum: 5 }
              }
            },
            stay: {
              bsonType: 'object',
              required: ['check_in', 'check_out', 'guests'],
              properties: {
                check_in: { bsonType: 'date' },
                check_out: { bsonType: 'date' },
                guests: { bsonType: 'int', minimum: 1, maximum: 10 }
              }
            },
            status: { enum: ['active', 'cancelled'] },
            created_at: { bsonType: 'date' },
            cancelled_at: { bsonType: ['date', 'null'] },
            status_history: {
              bsonType: 'array',
              minItems: 1,
              items: {
                bsonType: 'object',
                required: ['status', 'changed_at', 'comment'],
                properties: {
                  status: { enum: ['active', 'cancelled'] },
                  changed_at: { bsonType: 'date' },
                  comment: { bsonType: 'string', minLength: 1, maxLength: 255 }
                }
              }
            }
          }
        }
      },
      {
        $expr: {
          $gt: ['$stay.check_out', '$stay.check_in']
        }
      }
    ]
  },
  validationLevel: 'strict',
  validationAction: 'error'
});

db.users.createIndex({ login: 1 }, { unique: true, name: 'ux_users_login' });
db.users.createIndex({ first_name: 1, last_name: 1 }, { name: 'ix_users_name' });

db.hotels.createIndex({ city: 1 }, { name: 'ix_hotels_city' });
db.hotels.createIndex({ created_by: 1 }, { name: 'ix_hotels_created_by' });
db.hotels.createIndex({ amenities: 1 }, { name: 'ix_hotels_amenities' });

db.bookings.createIndex({ user_id: 1, created_at: -1 }, { name: 'ix_bookings_user_created_at' });
db.bookings.createIndex({ hotel_id: 1, status: 1 }, { name: 'ix_bookings_hotel_status' });
db.bookings.createIndex(
  { hotel_id: 1, status: 1, 'stay.check_in': 1, 'stay.check_out': 1 },
  { name: 'ix_bookings_availability' }
);

const RUN_NEGATIVE_TESTS = false;

if (RUN_NEGATIVE_TESTS) {
  try {
    db.users.insertOne({
      login: 'bad login with spaces',
      created_at: new Date()
    });
  } catch (error) {
    print('Validation test for users collection works:');
    print(error.message);
  }
}
