//
//  iosWeatherApp.swift
//  iosWeather
//
//  Created by Alessio Tamburro on 3/4/24.
//

import SwiftUI

@main
struct iosWeatherApp: App {
    
    @StateObject var modelView = ScanModelView(useCase: CentralUseCase())
    
    var body: some Scene {
        WindowGroup {
            NavigationStack {
                ScanView(modelView: modelView)
            }
        }
    }
}
