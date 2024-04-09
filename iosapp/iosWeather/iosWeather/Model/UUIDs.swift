//
//  UUIDs.swift
//  iosWeather
//
//  Created by Alessio Tamburro on 3/4/24.
//


import Foundation
import CoreBluetooth

enum UUIDs {
    //static let ledService = CBUUID(string: "4c40dfe1-7044-4058-8d53-24372c4e2e08")
    //static let ledStatusCharacteristic = CBUUID(string:  "4c40dfe1-7044-4058-8d53-24372c4e2e08") // Write
    /*
    static let temperatureService = CBUUID(string: "aaa691a2-3713-46ed-8d85-3a120d760175")
    static let temperatureCharacteristic = CBUUID(string:  "aaa691a2-3713-46ed-8d85-3a120d760175") // Read | Notify

    static let modeloutputService = CBUUID(string: "4c40dfe1-7044-4058-8d53-24372c4e2e08")
    static let modeloutputCharacteristic = CBUUID(string:  "4c40dfe1-7044-4058-8d53-24372c4e2e08") // Read | Notify
    */
    static let weatherService = CBUUID(string: "3c2312f7-7c24-4104-85ad-85a5a039d3cd")
    static let weatherCharacteristic = CBUUID(string:  "4c40dfe1-7044-4058-8d53-24372c4e2e08") // Read | Notify
    static let temperatureCharacteristic = CBUUID(string:  "aaa691a2-3713-46ed-8d85-3a120d760175") // Read | Notify
    static let humidityCharacteristic = CBUUID(string:  "d85e531e-0d16-4e39-902c-417b100e867e") // Read | Notify
}
