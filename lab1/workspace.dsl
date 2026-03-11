workspace "Hotel Booking System" "Variant 13 - hotel booking system" {

    model {
        guest = person "Guest" "Searches hotels, creates bookings, views and cancels own bookings."
        catalogManager = person "Catalog manager" "Creates hotels and maintains hotel catalog."
        supportSpecialist = person "Support specialist" "Searches users and reviews their bookings."

        emailService = softwareSystem "Email Delivery Service" "External service for transactional email delivery." "External"

        hotelBookingSystem = softwareSystem "Hotel Booking System" "System for hotel catalog management and hotel booking." {

            webApp = container "Web Application" "Web UI for guests and employees." "Web Application (React)" 
            apiGateway = container "API Gateway" "Single entry point for frontend clients and routing to internal services." "REST API / Reverse Proxy (Nginx + backend)"
            
            userService = container "User Service" "Manages users: create user, search by login, search by first and last name." "Java/Spring Boot REST API"
            hotelCatalogService = container "Hotel Catalog Service" "Manages hotels: create hotel, list hotels, search hotels by city." "Java/Spring Boot REST API"
            bookingService = container "Booking Service" "Creates bookings, returns user bookings, cancels bookings." "Java/Spring Boot REST API"
            notificationService = container "Notification Service" "Consumes booking events and sends email notifications." "Worker / Background Service"

            userDb = container "User Database" "Stores users." "PostgreSQL" "Database"
            hotelDb = container "Hotel Database" "Stores hotels." "PostgreSQL" "Database"
            bookingDb = container "Booking Database" "Stores bookings." "PostgreSQL" "Database"

            eventBus = container "Event Bus" "Transfers domain events between services." "RabbitMQ" "Queue"
        }

        guest -> hotelBookingSystem "Uses"
        catalogManager -> hotelBookingSystem "Uses"
        supportSpecialist -> hotelBookingSystem "Uses"
        hotelBookingSystem -> emailService "Sends booking notifications through"

        guest -> webApp "Searches hotels, creates and cancels bookings using" "HTTPS"
        catalogManager -> webApp "Creates hotels and browses hotel catalog using" "HTTPS"
        supportSpecialist -> webApp "Searches users and checks bookings using" "HTTPS"

        webApp -> apiGateway "Calls backend API" "HTTPS/REST"

        apiGateway -> userService "Routes user-related requests" "HTTPS/REST"
        apiGateway -> hotelCatalogService "Routes hotel-related requests" "HTTPS/REST"
        apiGateway -> bookingService "Routes booking-related requests" "HTTPS/REST"

        userService -> userDb "Reads from and writes to" "JDBC"
        hotelCatalogService -> hotelDb "Reads from and writes to" "JDBC"
        bookingService -> bookingDb "Reads from and writes to" "JDBC"

        bookingService -> hotelCatalogService "Checks hotel existence and availability" "HTTPS/REST"
        bookingService -> userService "Validates user data when needed" "HTTPS/REST"

        bookingService -> eventBus "Publishes BookingCreated / BookingCancelled events" "AMQP"
        eventBus -> notificationService "Delivers booking events" "AMQP"
        notificationService -> emailService "Sends confirmation/cancellation emails" "HTTPS/API"
        notificationService -> bookingDb "Stores notification status if needed" "JDBC"
    }

    views {
        systemContext hotelBookingSystem "SystemContext" "System Context diagram for Hotel Booking System" {
            include *
            autolayout lr
        }

        container hotelBookingSystem "Containers" "Container diagram for Hotel Booking System" {
            include *
            include guest
            include catalogManager
            include supportSpecialist
            include emailService
            autolayout lr
        }

        dynamic hotelBookingSystem "CreateBooking" "Dynamic view for the create booking use case" {
            guest -> webApp "1. Selects hotel and dates, confirms booking"
            webApp -> apiGateway "2. Sends POST /bookings"
            apiGateway -> bookingService "3. Routes booking request"
            bookingService -> hotelCatalogService "4. Requests hotel availability and validation"
            hotelCatalogService -> hotelDb "5. Reads hotel information"
            bookingService -> bookingDb "6. Creates booking record"
            bookingService -> eventBus "7. Publishes BookingCreated event"
            eventBus -> notificationService "8. Delivers BookingCreated event"
            notificationService -> emailService "9. Sends booking confirmation email"
            autolayout lr
        }
        theme default
    }
}