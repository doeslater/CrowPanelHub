package com.example.crowpanelhub

import android.os.Bundle
import androidx.activity.ComponentActivity
import androidx.activity.compose.setContent
import androidx.activity.enableEdgeToEdge
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.padding
import androidx.compose.material3.Scaffold
import androidx.compose.ui.Modifier
import androidx.navigation.compose.NavHost
import androidx.navigation.compose.composable
import androidx.navigation.compose.rememberNavController
import com.example.crowpanelhub.ui.theme.CrowPanelHubTheme
import dagger.hilt.android.AndroidEntryPoint

private const val MAIN_ROUTE = "main"
private const val DIAGNOSTICS_ROUTE = "diagnostics"

@AndroidEntryPoint
class MainActivity : ComponentActivity() {
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        enableEdgeToEdge()
        setContent {
            CrowPanelHubTheme {
                Scaffold(modifier = Modifier.fillMaxSize()) { innerPadding ->
                    val navController = rememberNavController()
                    NavHost(navController = navController, startDestination = MAIN_ROUTE) {
                        composable(MAIN_ROUTE) {
                            MainRoot(
                                onNavigateToDiagnostics = { navController.navigate(DIAGNOSTICS_ROUTE) },
                                modifier = Modifier.padding(innerPadding),
                            )
                        }
                        composable(DIAGNOSTICS_ROUTE) {
                            DiagnosticsRoot(modifier = Modifier.padding(innerPadding))
                        }
                    }
                }
            }
        }
    }
}
